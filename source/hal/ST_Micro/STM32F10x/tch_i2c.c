
/*
 * tch_i2c.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "tch_types.h"
#include "tch_hal.h"

#include "hal/tch_i2c.h"
#include "hal/tch_gpio.h"

#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/string.h"
#include "kernel/tch_interrupt.h"

#ifndef IIC_CLASS_KEY
#define IIC_CLASS_KEY							((uint16_t) 0x62D1)
#endif
#define TCH_IIC_BUSY_FLAG						((uint32_t) 0x10000)
#define TCH_IIC_ADDMOD_FLAG						((uint32_t) 0x20000)
#define TCH_IIC_MASTER_FLAG						((uint32_t) 0x40000)


#define TCH_IIC_PCLK_FREQ_MSK					((uint16_t) 0x1E)
#define TCH_IICQ_SIZE							((uint16_t) 0x6)

#define IIC_TX_HEADER							((uint16_t) 0xF0)
#define IIC_RX_HEADER							((uint16_t) 0xF1)

#define TCH_IIC_EVENT_ADDR_COMPLETE				((uint32_t) 0x1)
#define TCH_IIC_EVENT_TX_COMPLETE				((uint32_t) 0x2)
#define TCH_IIC_EVENT_RX_COMPLETE				((uint32_t) 0x4)
#define TCH_IIC_EVENT_IDLE						((uint32_t) 0x8)
#define TCH_IIC_EVENT_IOERROR					((uint32_t) 0x10)
#define TCH_IIC_EVENT_INVALID_STATE				((uint32_t) 0x20)
#define TCH_IIC_EVENT_ALL						((uint32_t) 0xFF)

#define IIC_IO_MAX_TIMEOUT						((uint32_t) 100)
#define IIC_SQ_ID_INIT							((uint32_t) 0)

#define SET_SAFE_RETURN()                       __SAFE_RETURN:
#define RETURN_SAFE()                           goto __SAFE_RETURN

#define IIC_isBusy(ins)                        ((tch_iic_handle_prototype*) ins)->status & TCH_IIC_BUSY_FLAG
#define IIC_setBusy(ins)                     do {\
		set_system_busy();\
		((tch_iic_handle_prototype*) ins)->status |= TCH_IIC_BUSY_FLAG;\
}while(0)

#define IIC_clrBusy(ins)                     do {\
		((tch_iic_handle_prototype*) ins)->status &= ~TCH_IIC_BUSY_FLAG;\
		clear_system_busy();\
}while(0)


#define IIC_isMaster(ins)                      ((tch_iic_handle_prototype*) ins)->status & TCH_IIC_MASTER_FLAG

#define IIC_setMaster(ins)                     do {\
		((tch_iic_handle_prototype*) ins)->status |= TCH_IIC_MASTER_FLAG;\
}while(0)

#define IIC_clrMaster(ins)                     do {\
		((tch_iic_handle_prototype*) ins)->status &= TCH_IIC_MASTER_FLAG;\
}while(0)


#define IIC_isAddr10B(ins)                     ((tch_iic_handle_prototype*) ins)->status & TCH_IIC_ADDMOD_FLAG

#define IIC_setAddr10B(ins)                    do {\
		((tch_iic_handle_prototype*) ins)->status |= TCH_IIC_ADDMOD_FLAG;\
}while(0)

#define IIC_clrAddr10B(ins)                    do {\
		((tch_iic_handle_prototype*) ins)->status &= ~TCH_IIC_ADDMOD_FLAG;\
}while(0)

typedef enum {
	IIc_MasterTx    =  ((uint8_t) 1),
	IIc_MasterRx    =  ((uint8_t) 3),
	IIc_SlaveTx     =  ((uint8_t) 5),
	IIc_SlaveRx     =  ((uint8_t) 7),
}msgtype;


typedef struct tch_iic_handle_prototype_t tch_iic_handle_prototype;
typedef struct tch_iic_op_desc_t {
	msgtype  mtype;
	uint16_t addr;
	BOOL     isDMA;
	uint8_t* bp;
	int32_t sz;
} tch_iicOpDesc;


struct tch_iic_handle_prototype_t {
	tch_iicHandle_t                        pix;
	tch_iic                              iic;
	uint32_t                             status;
	tch_gpioHandle*                      iohandle;
	tch_dmaHandle*                       rxdma;
	tch_dmaHandle*                       txdma;
	tch_eventId                          evId;
	const tch_core_api_t*                           env;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
	uint32_t                             isr_msg;
};


__USER_API__ static void tch_IIC_initConfig(tch_iicCfg* cfg);
__USER_API__ static tch_iicHandle_t* tch_IIC_alloc(const tch_core_api_t* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt);

__USER_API__ static tchStatus tch_IIC_close(tch_iicHandle_t* self);
__USER_API__ static tchStatus tch_IIC_writeMaster(tch_iicHandle_t* self,uint16_t addr,const void* wb,int32_t sz);
__USER_API__ static uint32_t tch_IIC_readMaster(tch_iicHandle_t* self,uint16_t addr,void* rb,int32_t sz,uint32_t timeout);
__USER_API__ static tchStatus tch_IIC_writeSlave(tch_iicHandle_t* self,uint16_t addr,const void* wb,int32_t sz);
__USER_API__ static uint32_t tch_IIC_readSlave(tch_iicHandle_t* self,uint16_t addr,void* rb,int32_t sz,uint32_t timeout);

static int tch_IIC_init(void);
static void tch_IIC_exit(void);


static BOOL tch_IIC_handleMasterEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);
static BOOL tch_IIC_handleSlaveEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);

static BOOL tch_IIC_handleMasterError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);
static BOOL tch_IIC_handleSlaveError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);

static BOOL tch_IIC_handleEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);
static BOOL tch_IIC_handleError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);

static void tch_IIC_validate(tch_iic_handle_prototype* hnd);
static BOOL tch_IIC_isValid(tch_iic_handle_prototype* hnd);
static void tch_IIC_invalidate(tch_iic_handle_prototype* hnd);


__USER_RODATA__ tch_hal_module_iic_t IIC_Ops = {
		.count = MFEATURE_IIC,
		.initCfg = tch_IIC_initConfig,
		.allocIIC = tch_IIC_alloc
};

static tch_mtxCb 	lock;
static tch_condvCb  condv;
static tch_hal_module_dma_t* dma;




static int tch_IIC_init(void)
{
	tch_mutexInit(&lock);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_IIC,IIC_CLASS_KEY,&IIC_Ops,FALSE);
}

static void tch_IIC_exit(void)
{
	tch_kmod_unregister(MODULE_TYPE_IIC,IIC_CLASS_KEY);
	tch_mutexDeinit(&lock);
	tch_condvDeinit(&condv);
}

MODULE_INIT(tch_IIC_init);
MODULE_EXIT(tch_IIC_exit);

__USER_API__ static tch_iicHandle_t* tch_IIC_alloc(const tch_core_api_t* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	if(!(i2c < MFEATURE_IIC))
		return NULL;

	if(!dma)
		dma = tch_kmod_request(MODULE_TYPE_DMA);

	tch_DmaCfg dmaCfg;
	tch_iic_descriptor* iicDesc = &IIC_HWs[i2c];
	const tch_iic_bs_t* iicbs = &IIC_BD_CFGs[i2c];
	I2C_TypeDef* iicHw = iicDesc->_hw;
	tch_iic_handle_prototype* ins = NULL;

	tch_hal_module_gpio_t* gpio = (tch_hal_module_gpio_t* ) Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
		return NULL;


	tchStatus result = tchOK;
	if((result = env->Mtx->lock(&lock,timeout)) != tchOK)
		return NULL;

	if(iicDesc->_handle)
	{
		iicDesc->sh_cnt++;
		env->Mtx->unlock(&lock);
		return iicDesc->_handle;
	}
	iicDesc->_handle = ins = (tch_iic_handle_prototype*) env->Mem->alloc(sizeof(tch_iic_handle_prototype));
	iicDesc->sh_cnt = 0;
	if((result = env->Mtx->unlock(&lock)) != tchOK)
		return NULL;


	iicDesc->_handle = ins;
	mset(ins,0,sizeof(tch_iic_handle_prototype));
	ins->iic = i2c;
	ins->env = env;
	ins->condv = env->Condv->create();
	ins->mtx = env->Mtx->create();
	ins->evId = env->Event->create();
	ins->rxdma = NULL;
	ins->txdma = NULL;

	if((iicbs->rxdma != DMA_NOT_USED) && dma)
	{
		dmaCfg.BufferType = DMA_BufferMode_Normal;
		dmaCfg.Ch = iicbs->rxch;
		dmaCfg.Dir = DMA_Dir_PeriphToMem;
		dmaCfg.FlowCtrl = DMA_FlowControl_DMA;
		dmaCfg.Priority = DMA_Prior_Mid;
		dmaCfg.mAlign = DMA_DataAlign_Byte;
		dmaCfg.mBurstSize= DMA_Burst_Single;
		dmaCfg.mInc = TRUE;
		dmaCfg.pAlign = DMA_DataAlign_Byte;
		dmaCfg.pBurstSize = DMA_Burst_Single;
		dmaCfg.pInc = FALSE;
		ins->rxdma = dma->allocate(env,iicbs->rxdma,&dmaCfg,timeout,popt);
	}

	if((iicbs->txdma != DMA_NOT_USED) && dma)
	{
		dmaCfg.BufferType = DMA_BufferMode_Normal;
		dmaCfg.Ch = iicbs->txch;
		dmaCfg.Dir = DMA_Dir_MemToPeriph;
		dmaCfg.FlowCtrl = DMA_FlowControl_DMA;
		dmaCfg.Priority = DMA_Prior_Mid;
		dmaCfg.mAlign = DMA_DataAlign_Byte;
		dmaCfg.mBurstSize= DMA_Burst_Single;
		dmaCfg.mInc = TRUE;
		dmaCfg.pAlign = DMA_DataAlign_Byte;
		dmaCfg.pBurstSize = DMA_Burst_Single;
		dmaCfg.pInc = FALSE;
		ins->txdma = dma->allocate(env,iicbs->txdma,&dmaCfg,timeout,popt);
	}

	gpio_config_t iocfg;
	gpio->initCfg(&iocfg);
	iocfg.Af = iicbs->afv;
	iocfg.Mode = GPIO_Mode_AF;
	iocfg.Speed = GPIO_OSpeed_25M;
	iocfg.popt = popt;
	ins->iohandle = gpio->allocIo(env,iicbs->port,((1 << iicbs->scl) | (1 << iicbs->sda)),&iocfg,timeout);


	*iicDesc->_clkenr |= iicDesc->clkmsk;
	if(popt == ActOnSleep)
		*iicDesc->_lpclkenr |= iicDesc->lpclkmsk;

	*iicDesc->_rstr |= iicDesc->rstmsk;
	*iicDesc->_rstr &= ~iicDesc->rstmsk;
	iicHw->CR1 |= I2C_CR1_SWRST;   // reset i2c peripheral
	iicHw->CR1 &= ~I2C_CR1_SWRST;


	iicHw->CR2 |= (I2C_CR2_ITERREN | (TCH_IIC_PCLK_FREQ_MSK & 0));   // set err & event interrupt enable; set pclk to 20MHz *** Not Affect to I2C Clk Frequency : it seems to be 30 MHz default
	if(cfg->AddrMode == IIC_ADDRMODE_10B)
	{
		iicHw->OAR1 |= I2C_OAR1_ADDMODE;
		iicHw->OAR1 |= ((cfg->Addr & 0x3FF) | (3 << 14));
	}
	else if(cfg->AddrMode == IIC_ADDRMODE_7B)
	{
		iicHw->OAR1 &= ~I2C_OAR1_ADDMODE;
		iicHw->OAR1 |= (((cfg->Addr << 1) & 0xFF) | (1 << 14));
	}

	iicHw->CCR &= ~0xFFF;
	// set I2C Op Mode (Standard or Fast)
	if(cfg->OpMode == IIC_OPMODE_FAST)
	{
		iicHw->CCR |= (I2C_CCR_FS | I2C_CCR_DUTY);
		switch(cfg->Baudrate)
		{
		case IIC_BAUDRATE_HIGH:
			iicHw->CCR |= (0xFFF &  3);
			break;
		case IIC_BAUDRATE_MID:
			iicHw->CCR |= (0xFFF &  6);
			break;
		case IIC_BAUDRATE_LOW:
			iicHw->CCR |= (0xFFF &  12);
			break;
		}
	}
	else
	{
		iicHw->CCR &= ~I2C_CCR_FS;
		switch(cfg->Baudrate){
		case IIC_BAUDRATE_HIGH:
			iicHw->CCR |= (0xFFF &  150);
			break;
		case IIC_BAUDRATE_MID:
			iicHw->CCR |= (0xFFF &  300);
			break;
		case IIC_BAUDRATE_LOW:
			iicHw->CCR |= (0xFFF &  600);
			break;
		}
	}

	ins->pix.close = tch_IIC_close;

	switch(cfg->Role)
	{
	case IIC_ROLE_MASTER:
		ins->pix.read = tch_IIC_readMaster;
		ins->pix.write = tch_IIC_writeMaster;
		IIC_setMaster(ins);
		break;
	case IIC_ROLE_SLAVE:
		ins->pix.read = tch_IIC_readSlave;
		ins->pix.write = tch_IIC_writeSlave;
		IIC_clrMaster(ins);
		break;
	}

	iicHw->CR1 |= I2C_CR1_STOP;
	// TODO : dummy handler consider isr remap
	tch_enableInterrupt(iicDesc->irq,PRIORITY_2, NULL);
	tch_IIC_validate(ins);
	return (tch_iicHandle_t*) ins;
}

__USER_API__ static tchStatus tch_IIC_close(tch_iicHandle_t* self)
{
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	tch_iic_descriptor* iicDesc = &IIC_HWs[ins->iic];

	tchStatus result = tchOK;
	if(!ins)
		return tchErrorParameter;
	if(!tch_IIC_isValid(ins))
		return tchErrorParameter;
	const tch_core_api_t* env = ins->env;
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;

	if((result = env->Mtx->lock(&lock,tchWaitForever)) != tchOK)
		return result;

	if(iicDesc->sh_cnt)
	{
		iicDesc->sh_cnt--;
		env->Mtx->unlock(&lock);
		return tchOK;
	}

	if((result = env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
		return result;

	while(IIC_isBusy(ins))
	{
		if((result = env->Condv->wait(ins->condv,ins->mtx,tchWaitForever)) != tchOK)
			return result;
	}
	IIC_setBusy(ins);
	tch_IIC_invalidate(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);
	env->Event->destroy(ins->evId);

	if(dma)
	{
		dma->freeDma(ins->rxdma);
		dma->freeDma(ins->txdma);
	}

	iicHw->CR1 |= I2C_CR1_SWRST;
	iicHw->CR1 &= ~I2C_CR1_PE;


	*iicDesc->_rstr |= iicDesc->rstmsk;
	*iicDesc->_clkenr &= ~iicDesc->clkmsk;
	*iicDesc->_lpclkenr &= ~iicDesc->lpclkmsk;
	tch_disableInterrupt(iicDesc->irq);


	iicDesc->_handle = NULL;
	IIC_clrBusy(ins);
	env->Mtx->unlock(&lock);
	env->Mem->free(ins);
	return tchOK;
}


__USER_API__ static void tch_IIC_initConfig(tch_iicCfg* cfg){
	cfg->Baudrate = IIC_BAUDRATE_MID;
	cfg->Role = IIC_ROLE_SLAVE;
	cfg->OpMode = IIC_OPMODE_STANDARD;
	cfg->Filter = FALSE;
	cfg->Addr = 0;
	cfg->AddrMode = IIC_ADDRMODE_7B;
}



__USER_API__ static tchStatus tch_IIC_writeMaster(tch_iicHandle_t* self,uint16_t addr,const void* wb,int32_t sz){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	tchStatus result = tchOK;
	int32_t sig;
	size_t idx = 0;
	if(!ins)
		return tchErrorParameter;
	if(!tch_IIC_isValid(ins))
		return tchErrorParameter;
	if(!IIC_isMaster(ins))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	I2C_TypeDef* iicHw = (I2C_TypeDef*) IIC_HWs[ins->iic]._hw;
	if((result = ins->env->Mtx->lock(ins->mtx,IIC_IO_MAX_TIMEOUT)) != tchOK)
		return result;

	while(IIC_isBusy(ins))
	{
		if((result = ins->env->Condv->wait(ins->condv,ins->mtx,IIC_IO_MAX_TIMEOUT)) != tchOK)
			return result;
	}
	IIC_setBusy(ins);
	if((result = ins->env->Mtx->unlock(ins->mtx)) != tchOK)
		return result;

	tch_iicOpDesc tx_req;
	tx_req.mtype = IIc_MasterTx;
	tx_req.sz = sz;
	tx_req.bp = ((uint8_t*) wb);
	tx_req.addr = addr;
	tx_req.isDMA = FALSE;
	ins->isr_msg = (uint32_t) &tx_req;

	iicHw->CR1 &= ~I2C_CR1_STOP;
	while(iicHw->CR1 & I2C_CR1_STOP)__NOP();

	if(ins->txdma)
	{
		iicHw->CR2 |= I2C_CR2_DMAEN;
		tx_req.isDMA = TRUE;
	}

	iicHw->CR2 |= I2C_CR2_ITEVTEN;

	iicHw->CR1 |= I2C_CR1_PE;
	iicHw->CR1 |= I2C_CR1_START;


	if(ins->txdma)
	{
		// prepare DMA request
		tch_DmaReqDef txreq;
		dma->initReq(&txreq, (uwaddr_t) wb, (uwaddr_t) &iicHw->DR,sz,DMA_Dir_MemToPeriph);

		// wait for addressing complete
		if(ins->env->Event->wait(ins->evId,TCH_IIC_EVENT_ADDR_COMPLETE,IIC_IO_MAX_TIMEOUT) != tchOK)
		{
			ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
			RETURN_SAFE();
		}

		// start DMA transfer
		if(dma->beginXferSync(ins->txdma,&txreq,IIC_IO_MAX_TIMEOUT,&result))
		{
			ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
			RETURN_SAFE();
		}

		iicHw->CR2 &= ~I2C_CR2_DMAEN;
		if(ins->env->Event->wait(ins->evId,TCH_IIC_EVENT_IDLE,IIC_IO_MAX_TIMEOUT) != tchOK)
		{
			ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
			RETURN_SAFE();
		}

	}
	else
	{
		// wait data transfer complete
		if(ins->env->Event->wait(ins->evId,(TCH_IIC_EVENT_IDLE | TCH_IIC_EVENT_TX_COMPLETE),IIC_IO_MAX_TIMEOUT) != tchOK)
		{
			ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
			RETURN_SAFE();
		}
	}

	sig = ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);

	if(sig & TCH_IIC_EVENT_IOERROR)
	{
		result = tchErrorIo;
		RETURN_SAFE();
	}

	result = tchOK;
	SET_SAFE_RETURN()

	while(iicHw->CR1 & I2C_CR1_STOP) __NOP();
	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	while(iicHw->SR2 & 7) __NOP();
	iicHw->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
	iicHw->CR1 &= ~I2C_CR1_PE;
	IIC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return result;
}

__USER_API__ static uint32_t tch_IIC_readMaster(tch_iicHandle_t* self,uint16_t addr,void* rb,int32_t sz,uint32_t timeout){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	tchStatus result = tchOK;
	int32_t sig;

	if((!self) || (!addr) || (!sz))
		return 0;
	if(!tch_IIC_isValid(ins))
		return 0;
	if(!IIC_isMaster(ins))
		return 0;
	if(tch_port_isISR())
		return 0;

	I2C_TypeDef* iicHw = IIC_HWs[ins->iic]._hw;
	if((result = ins->env->Mtx->lock(ins->mtx,timeout)) != tchOK)
		return 0;

	while(IIC_isBusy(ins))
	{
		if((result = ins->env->Condv->wait(ins->condv,ins->mtx,timeout)) != tchOK)
			return 0;
	}
	IIC_setBusy(ins);
	if((result = ins->env->Mtx->unlock(ins->mtx)) != tchOK)
		return 0;

	tch_iicOpDesc rx_req;
	rx_req.mtype = IIc_MasterRx;
	rx_req.sz = sz;
	rx_req.bp = (uint8_t*) rb;
	rx_req.addr = addr;
	rx_req.isDMA = FALSE;
	ins->isr_msg = (uint32_t) &rx_req;

	while(iicHw->CR1 & I2C_CR1_STOP) __NOP();

	iicHw->CR1 |= (I2C_CR1_ACK | I2C_CR1_PE);

	if(ins->rxdma)
	{
		rx_req.isDMA = TRUE;
		iicHw->CR2 |= (I2C_CR2_DMAEN | I2C_CR2_LAST);
	}

	iicHw->CR2 |= I2C_CR2_ITEVTEN;
	iicHw->CR1 |= I2C_CR1_START;

	if(ins->rxdma)
	{
		tch_DmaReqDef rxreq;
		dma->initReq(&rxreq,(uwaddr_t) rb, (uwaddr_t) &iicHw->DR, sz, DMA_Dir_PeriphToMem);

		// wait for addressing complete
		if((result = ins->env->Event->wait(ins->evId,TCH_IIC_EVENT_ADDR_COMPLETE,IIC_IO_MAX_TIMEOUT)) != tchOK)
		{
			sig = ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
			RETURN_SAFE();
		}

		iicHw->CR2 &= ~I2C_CR2_ITEVTEN;
		sz -= dma->beginXferSync(ins->rxdma,&rxreq,IIC_IO_MAX_TIMEOUT,&result);
		iicHw->CR1 |= I2C_CR1_STOP;
		iicHw->CR2 &= ~(I2C_CR2_DMAEN | I2C_CR2_LAST);
	}
	else
	{
		// wait data transfer complete
		ins->env->Event->wait(ins->evId,(TCH_IIC_EVENT_IDLE | TCH_IIC_EVENT_RX_COMPLETE),tchWaitForever);
		sz -= rx_req.sz;
	}

	ins->env->Event->clear(ins->evId,TCH_IIC_EVENT_ALL);
	SET_SAFE_RETURN()

	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	while(iicHw->SR2 & 7) __NOP();
	iicHw->CR2 &= ~(I2C_CR2_ITEVTEN | I2C_CR2_ITBUFEN);
	iicHw->CR1 &= ~I2C_CR1_PE;
	IIC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return sz;
}


__USER_API__ static tchStatus tch_IIC_writeSlave(tch_iicHandle_t* self,uint16_t addr,const void* wb,int32_t sz){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	if((!self) || (!addr) || (!sz))
		return tchErrorParameter;
	if(!tch_IIC_isValid(ins))
		return tchErrorParameter;
	if(IIC_isMaster(ins))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	tchEvent evt;
	tch_iicOpDesc tx_msg;
	tx_msg.bp = (uint8_t*) wb;
	tx_msg.sz = sz;
	tx_msg.mtype = IIc_SlaveTx;
	const tch_core_api_t* env = ins->env;

	I2C_TypeDef* iicHw = (I2C_TypeDef*) IIC_HWs[ins->iic]._hw;
	if((evt.status = ins->env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
		return evt.status;
	while(IIC_isBusy(ins)){
		if((evt.status = ins->env->Condv->wait(ins->condv,ins->mtx,tchWaitForever)) != tchOK)
			return evt.status;
	}
	IIC_setBusy(ins);
	if((evt.status = ins->env->Mtx->unlock(ins->mtx)) != tchOK)
		return evt.status;
	iicHw->CR2 |= I2C_CR2_ITEVTEN;




	evt.status = tchOK;
	SET_SAFE_RETURN()
	iicHw->CR2 &= ~(I2C_CR2_ITBUFEN | I2C_CR2_ITEVTEN);
	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	IIC_clrBusy(ins);
	ins->env->Condv->wake(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return evt.status;

}

__USER_API__ static uint32_t tch_IIC_readSlave(tch_iicHandle_t* self,uint16_t addr,void* rb,int32_t sz,uint32_t timeout){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	if((!self) || (!addr) || (!sz))
		return tchErrorParameter;
	if(!tch_IIC_isValid(ins))
		return tchErrorParameter;
	if(IIC_isMaster(ins))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	tchEvent evt;


	evt.status = tchOK;
	SET_SAFE_RETURN()

	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	IIC_clrBusy(ins);
	ins->env->Condv->wake(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return evt.status;
}

static void tch_IIC_validate(tch_iic_handle_prototype* hnd){
	hnd->status &= ~0xFFFF;
	hnd->status |= ((uint32_t) hnd & 0xFFFF) ^ IIC_CLASS_KEY;
}

static BOOL tch_IIC_isValid(tch_iic_handle_prototype* hnd){
	return (hnd->status & 0xFFFF) == ((uint32_t) hnd & 0xFFFF) ^ IIC_CLASS_KEY;
}

static void tch_IIC_invalidate(tch_iic_handle_prototype* hnd){
	hnd->status &= ~0xFFFF;
}

static BOOL tch_IIC_handleMasterEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	tch_iicOpDesc* iic_req = (tch_iicOpDesc*) ins->isr_msg;
	uint32_t rout;
	uint16_t sr1 = 0;
	uint16_t sr2 = 0;
	BOOL isDma = FALSE;
	if(!ins)
		return FALSE;
	sr1 = iicHw->SR1;
	const tch_core_api_t* env = ins->env;
	switch(iic_req->mtype){
	case IIc_MasterRx:
		if(sr1 & I2C_SR1_SB){
			if(IIC_isAddr10B(ins)){   // * write first byte address
				iicHw->DR = (IIC_RX_HEADER | ((iic_req->addr >> 7) & 7));
			}else{
				iicHw->DR =  (0xFF & (iic_req->addr | 1));
			}
		}
		if(sr1 & I2C_SR1_ADDR){							// * read SR1
			sr2 = iicHw->SR2;							// * read SR2
			if(sr2 & I2C_SR2_TRA){							// * if communication mode is not transmission
				env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);	// * report error and wakeup waiting thread
				return FALSE;
			}
			if(iic_req->sz == 1){
				iicHw->CR1 &= ~I2C_CR1_ACK;         // if data to receive is single-byte, ACK bit will be cleared in CR1
			}
			sr1 &= ~I2C_SR1_ADDR;					// mark 'ADDR' as handled
			if(!iic_req->isDMA){					// if not DMA mode
				iicHw->CR2 |= I2C_CR2_ITBUFEN;						// enable buffer event interrrupt to handle
			}else{
				env->Event->set(ins->evId,TCH_IIC_EVENT_ADDR_COMPLETE);// otherwise wakeup wating thread and allow it to start DMA
				return TRUE;
			}
		}
		if(sr1 & I2C_SR1_RXNE){
			if(iic_req->sz > 0){
				iic_req->sz--;
				if(iic_req->sz == 0){
					iicHw->CR1 &= ~I2C_CR1_ACK;
					env->Event->set(ins->evId,(TCH_IIC_EVENT_RX_COMPLETE | TCH_IIC_EVENT_IDLE));
				}
				*((uint8_t*) iic_req->bp++) = iicHw->DR;	// read data
			}
			if(iic_req->sz < 1)						// if second last byte received, set STOP bit
			{
				rout = iicHw->DR;
				iicHw->CR1 |= I2C_CR1_STOP;
			}
			return TRUE;
		}

		if(sr1 & I2C_SR1_BTF){
			if(iic_req->sz){
				*((uint8_t*) iic_req->bp++) = iicHw->DR;	// read date into buffer
			}else{
				env->Event->set(ins->evId,TCH_IIC_EVENT_RX_COMPLETE);
				return TRUE;
			}
		}
		if(sr1 & I2C_SR1_TXE){
			env->Event->set(ins->evId,TCH_IIC_EVENT_INVALID_STATE);
			return TRUE;
		}
		if(sr1 & I2C_SR1_ADD10){   // * write second byte address
			iicHw->DR = (0xFF & iic_req->addr);
			sr1 &= ~I2C_SR1_ADD10;
		}
		break;
	case IIc_MasterTx:
		if(sr1 & I2C_SR1_SB){
			if(IIC_isAddr10B(ins)){
				iicHw->DR = (IIC_TX_HEADER | ((iic_req->addr >> 7) & 7));  // in 10-bit address tx mode, transmit tx header and addr
			}else{
				iicHw->DR = (0xFF & (iic_req->addr & ~1));                  // in 7-bit address tx mode, transmit address with reset LSB
			}
		}
		if(sr1 & I2C_SR1_RXNE){
			env->Event->set(ins->evId,TCH_IIC_EVENT_INVALID_STATE);
			return TRUE;
		}
		if(sr1 & I2C_SR1_ADDR){
			sr2 = iicHw->SR2;							// * read SR2
			if(!(sr2 & I2C_SR2_TRA)){							// * if communication mode is not transmission
				env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);	// * report error and wakeup waiting thread
				return FALSE;
			}
			sr1 &= ~I2C_SR1_ADDR;					// mark 'ADDR' as handled
			if(!iic_req->isDMA){					// if not DMA mode
				iicHw->CR2 |= I2C_CR2_ITBUFEN;						// enable buffer event interrrupt to handle
			}else{
				env->Event->set(ins->evId,TCH_IIC_EVENT_ADDR_COMPLETE);// otherwise wakeup wating thread and allow it to start DMA
				return TRUE;
			}
		}
		if(sr1 & I2C_SR1_BTF){
			if((sr1 & I2C_SR1_TXE) && (iic_req->sz == 0)){
				iicHw->CR1 |= I2C_CR1_STOP;
				env->Event->set(ins->evId,TCH_IIC_EVENT_IDLE);
				return TRUE;
			}
			if(iic_req->isDMA){
				iicHw->CR1 |= I2C_CR1_STOP;
				env->Event->set(ins->evId,TCH_IIC_EVENT_IDLE);
				return TRUE;
			}
		}
		if(sr1 & I2C_SR1_TXE){
			if(iic_req->sz > 0){
				iic_req->sz--;
				iicHw->DR = *((uint8_t*) iic_req->bp++);
			}else{
				env->Event->set(ins->evId,TCH_IIC_EVENT_TX_COMPLETE);
				return TRUE;
			}
		}
		if(sr1 & I2C_SR1_ADD10){
			iicHw->DR = (0xFF & iic_req->addr);
			sr1 &= ~I2C_SR1_ADD10;
		}
		break;
	}

	return FALSE;
}

static BOOL tch_IIC_handleSlaveEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){

}

static BOOL tch_IIC_handleMasterError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	if(!ins)
		return FALSE;
	if(!tch_IIC_isValid(ins))
		return FALSE;
	const tch_core_api_t* env = ins->env;
	if(iicHw->SR1 & I2C_SR1_PECERR)
	{
		env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);
		return TRUE;
	}

	if(iicHw->SR1 & I2C_SR1_BERR)
	{
		env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);
		return TRUE;
	}

	if(iicHw->SR1 & I2C_SR1_TIMEOUT)
	{
		env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);
		return TRUE;
	}

	if(iicHw->SR1 & I2C_SR1_OVR)
	{
		env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);
		return TRUE;
	}

	if(iicHw->SR1 & I2C_SR1_ARLO)
	{
		env->Event->set(ins->evId,TCH_IIC_EVENT_IOERROR);
		return TRUE;
	}

	if(iicHw->SR1 & I2C_SR1_AF)
	{
		iicHw->CR1 |= I2C_CR1_STOP;
		env->Event->set(ins->evId,TCH_IIC_EVENT_IDLE);
		return TRUE;
	}
	return FALSE;

}

static BOOL tch_IIC_handleSlaveError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){

}


static BOOL tch_IIC_handleEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	tch_iicOpDesc* isr_req = NULL;
	uint16_t sr;
	if(!ins)
		return FALSE;
	if(!tch_IIC_isValid(ins))
		return FALSE;
	if(IIC_isMaster(ins))
		return tch_IIC_handleMasterEvent(ins,iicDesc);
	else
		return tch_IIC_handleSlaveEvent(ins,iicDesc);
	// unhandled hw interrupt in this case peripheral reset
	return FALSE;

}

static BOOL tch_IIC_handleError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	if(!ins)
		return FALSE;
	if(!tch_IIC_isValid(ins))
		return FALSE;

	if(IIC_isMaster(ins))
		return tch_IIC_handleMasterError(ins,iicDesc);
	else
		return tch_IIC_handleSlaveError(ins,iicDesc);

	return FALSE;

}


void I2C1_EV_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[0];
	tch_IIC_handleEvent((tch_iic_handle_prototype*)iicDesc->_handle,iicDesc);
}

void I2C1_ER_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[0];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}

void I2C2_EV_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[1];
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) iicDesc->_handle;
	tch_IIC_handleEvent(ins,iicDesc);
}

void I2C2_ER_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[1];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}

void IC3_EV_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[2];
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) iicDesc->_handle;
	tch_IIC_handleEvent(ins,iicDesc);
}

void I2C3_ER_IRQHandler(void){
	tch_iic_descriptor* iicDesc = &IIC_HWs[2];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}
