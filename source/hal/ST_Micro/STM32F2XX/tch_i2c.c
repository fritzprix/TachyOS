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


#include "hal/tch_hal.h"
#include "tch_i2c.h"
#include "tch_halInit.h"
#include "tch_halcfg.h"


typedef struct tch_iic_handle_prototype_t tch_iic_handle_prototype;

#define TCH_IIC_CLASS_KEY                      ((uint16_t) 0x62D1)
#define TCH_IIC_BUSY_FLAG                      ((uint16_t) 0x10000)
#define TCH_IIC_ADDMOD_FLAG                    ((uint16_t) 0x20000)

#define IIC_isBusy(ins)                        ((tch_iic_handle_prototype*) ins)->status & TCH_IIC_BUSY_FLAG
#define IIC_setBusy(ins)                     do {\
	((tch_iic_handle_prototype*) ins)->status |= TCH_IIC_BUSY_FLAG;\
}while(0)

#define IIC_clrBusy(ins)                     do {\
	((tch_iic_handle_prototype*) ins)->status &= ~TCH_IIC_BUSY_FLAG;\
}while(0)



#define TCH_IIC_OCCP_MSK                       ((uint32_t) 0x0010)
#define TCH_IIC_PCLK_FREQ_MSK                  ((uint16_t) 0x1E)

#define TCH_IICQ_SIZE                          ((uint16_t) 0x6)

#define IIC_TX_HEADER                          ((uint16_t) 0xF0)
#define IIC_RX_HEADER                          ((uint16_t) 0xF1)





struct tch_iic_handle_prototype_t {
	tch_iicHandle                        pix;
	tch_iic                              iic;
	uint32_t                             status;
	tch_GpioHandle*                      iohandle;
	tch_DmaHandle*                       rxdma;
	tch_DmaHandle*                       txdma;
	tch_msgqId                           mq;
	const tch*                           env;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
};


static void tch_IIC_initCfg(tch_iicCfg* cfg);
static tch_iicHandle* tch_IIC_alloc(const tch* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt);

static tchStatus tch_IIC_close(tch_iicHandle* self);
static tchStatus tch_IIC_writeMaster(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz);
static tchStatus tch_IIC_readMaster(tch_iicHandle* self,uint16_t addr,void* rb,size_t sz,uint32_t timeout);
static tchStatus tch_IIC_writeSlave(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz);
static tchStatus tch_IIC_readSlave(tch_iicHandle* self,uint16_t addr,void* rb,size_t sz,uint32_t timeout);

static BOOL tch_IIC_handleEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);
static BOOL tch_IIC_handleError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc);

static void tch_IICValidate(tch_iic_handle_prototype* hnd);
static BOOL tch_IICisValid(tch_iic_handle_prototype* hnd);
static void tch_IICInvalidate(tch_iic_handle_prototype* hnd);

typedef struct tch_lld_i2c_prototype {
	tch_lld_iic                           pix;
	tch_mtxId                             mtx;
	tch_condvId                           condv;
} tch_lld_i2c_prototype;


/**
 */
__attribute__((section(".data"))) static tch_lld_i2c_prototype IIC_StaticInstance = {
		{
				tch_IIC_initCfg,
				tch_IIC_alloc
		},
		NULL,
		NULL

};


const tch_lld_iic* tch_i2c_instance = (const tch_lld_iic*) &IIC_StaticInstance;


static tch_iicHandle* tch_IIC_alloc(const tch* env,tch_iic i2c,tch_iicCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	if(!(i2c < MFEATURE_IIC))
		return NULL;
	if(!IIC_StaticInstance.mtx)
		IIC_StaticInstance.mtx = env->Mtx->create();
	if(!IIC_StaticInstance.condv)
		IIC_StaticInstance.condv = env->Condv->create();

	tch_DmaCfg dmaCfg;
	tch_iic_descriptor* iicDesc = &IIC_HWs[i2c];
	tch_iic_bs* iicbs = &IIC_BD_CFGs[i2c];
	I2C_TypeDef* iicHw = iicDesc->_hw;

	tchStatus result = osOK;
	if((result = env->Mtx->lock(IIC_StaticInstance.mtx,timeout)) != osOK)
		return NULL;
	while(iicDesc->_handle){
		if((result = env->Condv->wait(IIC_StaticInstance.condv,IIC_StaticInstance.mtx,timeout)) != osOK)
			return NULL;
	}
	iicDesc->_handle = (void*)TCH_IIC_OCCP_MSK;  // mark as occupied
	if((result = env->Mtx->unlock(IIC_StaticInstance.mtx)) != osOK)
		return NULL;

	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) env->Mem->alloc(sizeof(tch_iic_handle_prototype));
	env->uStdLib->string->memset(ins,0,sizeof(tch_iic_handle_prototype));
	ins->env = env;
	ins->condv = env->Condv->create();
	ins->mtx = env->Mtx->create();
	ins->mq = env->MsgQ->create(TCH_IICQ_SIZE);
	ins->rxdma = NULL;
	ins->txdma = NULL;
	if(iicbs->rxdma != DMA_NOT_USED){
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
		ins->rxdma = env->Device->dma->allocDma(env,iicbs->rxdma,&dmaCfg,timeout,popt);
	}

	if(iicbs->txdma != DMA_NOT_USED){
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
		ins->txdma = env->Device->dma->allocDma(env,iicbs->txdma,&dmaCfg,timeout,popt);
	}

	tch_GpioCfg iocfg;
	env->Device->gpio->initCfg(&iocfg);
	iocfg.Af = iicbs->afv;
	iocfg.Mode = GPIO_Mode_AF;
	ins->iohandle = env->Device->gpio->allocIo(env,iicbs->port,((1 << iicbs->scl) | (1 << iicbs->sda)),&iocfg,timeout,popt);


	*iicDesc->_clkenr |= iicDesc->clkmsk;
	if(popt == ActOnSleep)
		*iicDesc->_lpclkenr |= iicDesc->lpclkmsk;

	*iicDesc->_rstr |= iicDesc->rstmsk;
	*iicDesc->_rstr &= ~iicDesc->rstmsk;

	iicHw->CR1 |= I2C_CR1_SWRST;   // reset i2c peripheral
	iicHw->CR2 |= (I2C_CR2_ITEVTEN | I2C_CR2_ITERREN | (TCH_IIC_PCLK_FREQ_MSK & 20));   // set err & event interrupt enable; set pclk to 20MHz
	if(cfg->AddrMode == IIC_ADDRMODE_10B){
		iicHw->OAR1 |= I2C_OAR1_ADDMODE;
		iicHw->OAR1 |= ((cfg->Addr << 1) & 0xFE);
	}else if(cfg->AddrMode == IIC_ADDRMODE_7B){
		iicHw->OAR1 &= ~I2C_OAR1_ADDMODE;
		iicHw->OAR1 |= (cfg->Addr & 0x3FF);
	}

	iicHw->CCR &= ~0xFFF;
	// set I2C Op Mode (Standard or Fast)
	if(cfg->OpMode == IIC_OPMODE_FAST){
		iicHw->CCR |= I2C_CCR_FS;
		switch(cfg->Baudrate){
		case IIC_BAUDRATE_HIGH:
			/*
			 * Fast Mode : 400 kHz
			 */
			iicHw->CCR |= (0xFFF & 2);
			break;
		case IIC_BAUDRATE_MID:
			/*
			 * Fast Mode : 200 kHz
			 */
			iicHw->CCR |= (0xFFF & 4);
			break;
		case IIC_BAUDRATE_LOW:
			/*
			 *  Fast Mode : 100 kHz
			 */
			iicHw->CCR |= (0xFFF & 8);
			break;
		}
	}else{
		iicHw->CCR &= ~I2C_CCR_FS;
		switch(cfg->Baudrate){
		case IIC_BAUDRATE_HIGH:
			/*
			 * Standard Mode : 100 kHz
			 */
			iicHw->CCR |= (0xFFF & 100);
			break;
		case IIC_BAUDRATE_MID:
			/*
			 * Standard Mode : 50 kHz
			 */
			iicHw->CCR |= (0xFFF & 200);
			break;
		case IIC_BAUDRATE_LOW:
			/*
			 *  Standard Mode : 25 kHz
			 */
			iicHw->CCR |= (0xFFF & 400);
			break;
		}
	}

	ins->pix.close = tch_IIC_close;
	switch(cfg->Role){
	case IIC_ROLE_MASTER:
		ins->pix.read = tch_IIC_readMaster;
		ins->pix.write = tch_IIC_writeMaster;
		break;
	case IIC_ROLE_SLAVE:
		ins->pix.read = tch_IIC_readSlave;
		ins->pix.write = tch_IIC_writeSlave;
		break;
	}


	env->Device->interrupt->setPriority(iicDesc->irq,env->Device->interrupt->Priority.Normal);
	env->Device->interrupt->enable(iicDesc->irq);

	tch_IICValidate(ins);
	return (tch_iicHandle*) ins;
}

static tchStatus tch_IIC_close(tch_iicHandle* self){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	tch_iic_descriptor* iicDesc = &IIC_HWs[ins->iic];

	tchStatus result = osOK;
	if(!ins)
		return osErrorParameter;
	if(!tch_IICisValid(ins))
		return osErrorParameter;
	const tch* env = ins->env;

	if((result = env->Mtx->lock(ins->mtx,osWaitForever)) != osOK)
		return result;
	while(IIC_isBusy(ins)){
		if((result = env->Condv->wait(ins->condv,ins->mtx,osWaitForever)) != osOK)
			return result;
	}
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);
	env->MsgQ->destroy(ins->mq);
	env->Device->dma->freeDma(ins->rxdma);
	env->Device->dma->freeDma(ins->txdma);

	*iicDesc->_rstr |= iicDesc->rstmsk;
	*iicDesc->_clkenr &= ~iicDesc->clkmsk;
	*iicDesc->_lpclkenr &= ~iicDesc->lpclkmsk;

	if((result = env->Mtx->lock(IIC_StaticInstance.mtx,osWaitForever)) != osOK)
		return result;
	iicDesc->_handle = NULL;
	env->Condv->wake(IIC_StaticInstance.condv);
	env->Mtx->unlock(IIC_StaticInstance.mtx);
	env->Mem->free(ins);
	return osOK;
}


static void tch_IIC_initCfg(tch_iicCfg* cfg){
	cfg->Baudrate = IIC_BAUDRATE_MID;
	cfg->Role = IIC_ROLE_SLAVE;
	cfg->OpMode = IIC_OPMODE_STANDARD;
	cfg->Filter = FALSE;
	cfg->Addr = 0;
	cfg->AddrMode = IIC_ADDRMODE_7B;
}




static tchStatus tch_IIC_writeMaster(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz){
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) self;
	osEvent evt;
	size_t idx = 0;
	if(!ins)
		return osErrorParameter;
	if(!tch_IICisValid(ins))
		return osErrorParameter;
	I2C_TypeDef* iicHw = (I2C_TypeDef*) IIC_HWs[ins->iic]._hw;
	if((evt.status = ins->env->Mtx->lock(ins->mtx,osWaitForever)) != osOK)
		return evt.status;
	while(IIC_isBusy(ins)){
		if((evt.status = ins->env->Condv->wait(ins->condv,ins->mtx,osWaitForever)) != osOK)
			return evt.status;
	}
	IIC_setBusy(ins);
	if((evt.status = ins->env->Mtx->unlock(ins->mtx)) != osOK)
		return evt.status;


	iicHw->CR1 |= I2C_CR1_PE;   //enable i2c
	iicHw->CR1 |= I2C_CR1_START;
	evt = ins->env->MsgQ->get(ins->mq,osWaitForever);
	if((evt.status != osEventMessage) || (evt.value.v != I2C_SR1_SB))
		return osErrorValue;
	if(ins->status & TCH_IIC_ADDMOD_FLAG){
		/// 10bit addressing mode
		iicHw->DR = (IIC_TX_HEADER | (addr >> 8));
		evt = ins->env->MsgQ->get(ins->mq,osWaitForever);
		if((evt.status != osEventMessage) || (evt.value.v != I2C_SR1_ADD10))
			return osErrorValue;
		iicHw->CR2 |= I2C_CR2_ITBUFEN;
		iicHw->DR = (0xFF & addr);
		evt = ins->env->MsgQ->get(ins->mq,osWaitForever);
		if((evt.status != osEventMessage) || (evt.value.v != I2C_SR1_ADDR))
			return osErrorValue;
		for(idx = 0;idx < sz; idx++){
			evt = ins->env->MsgQ->get(ins->mq,osWaitForever);
			if((evt.status != osEventMessage) || (evt.value.v != I2C_SR1_TXE))
				return osErrorValue;
			iicHw->DR = wb[idx];
		}

	}else{
		/// 7bit addressing mode
	}

}

static tchStatus tch_IIC_readMaster(tch_iicHandle* self,uint16_t addr,void* rb,size_t sz,uint32_t timeout){

}

static tchStatus tch_IIC_writeSlave(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz){

}

static tchStatus tch_IIC_readSlave(tch_iicHandle* self,uint16_t addr,void* rb,size_t sz,uint32_t timeout){

}

static void tch_IICValidate(tch_iic_handle_prototype* hnd){
	hnd->status &= ~0xFFFF;
	hnd->status |= ((uint32_t) hnd & 0xFFFF) ^ TCH_IIC_CLASS_KEY;
}

static BOOL tch_IICisValid(tch_iic_handle_prototype* hnd){
	return (hnd->status & 0xFFFF) == ((uint32_t) hnd & 0xFFFF) ^ TCH_IIC_CLASS_KEY;
}

static void tch_IICInvalidate(tch_iic_handle_prototype* hnd){
	hnd->status &= ~0xFFFF;
}

static BOOL tch_IIC_handleEvent(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	if(!ins)
		return FALSE;
	if(iicHw->SR1 & I2C_SR1_RXNE){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_TXE){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_ADDR){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_ADD10){
		return TRUE;
	}

}

static BOOL tch_IIC_handleError(tch_iic_handle_prototype* ins,tch_iic_descriptor* iicDesc){
	I2C_TypeDef* iicHw = (I2C_TypeDef*) iicDesc->_hw;
	if(!ins)
		return FALSE;
	if(iicHw->SR1 & I2C_SR1_PECERR){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_BERR){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_TIMEOUT){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_OVR){
		return TRUE;
	}
	if(iicHw->SR1 & I2C_SR1_ARLO){
		return TRUE;
	}
}


void I2C1_EV_IRQHandler(void){
	/* I2C1 Event                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[0];
	tch_IIC_handleEvent((tch_iic_handle_prototype*)iicDesc->_handle,iicDesc);
}

void I2C1_ER_IRQHandler(void){
	/* I2C1 Error                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[0];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}

void I2C2_EV_IRQHandler(void){
	/* I2C2 Event                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[1];
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) iicDesc->_handle;
	tch_IIC_handleEvent(ins,iicDesc);
}

void I2C2_ER_IRQHandler(void){
	/* I2C2 Error                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[1];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}

void IC3_EV_IRQHandler(void){
	/* I2C3 event                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[2];
	tch_iic_handle_prototype* ins = (tch_iic_handle_prototype*) iicDesc->_handle;
	tch_IIC_handleEvent(ins,iicDesc);
}

void I2C3_ER_IRQHandler(void){
	/* I2C3 error                   */
	tch_iic_descriptor* iicDesc = &IIC_HWs[2];
	tch_IIC_handleError(iicDesc->_handle,iicDesc);
}
