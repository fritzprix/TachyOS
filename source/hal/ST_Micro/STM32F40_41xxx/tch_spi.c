/*
 * tch_spi.c
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

#include "tch_hal.h"
#include "tch_gpio.h"
#include "tch_spi.h"

#include "kernel/util/string.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"

typedef struct tch_lld_spi_prototype tch_lld_spi_prototype;
typedef struct tch_spi_handle_prototype tch_spi_handle_prototype;
typedef struct tch_spi_request_s tch_spi_request;

#define SPI_ERR_MSK						((uint32_t) 0xF0000000)     // SPI Error Msk
#define SPI_ERR_OVR						((uint32_t) 0x10000000)     // SPI Overrun Error
#define SPI_ERR_UDR						((uint32_t) 0x20000000)     // SPI Underrun Error

#ifndef SPI_CLASS_KEY
#define SPI_CLASS_KEY					((uint16_t) 0x41F5)
#endif

#define TCH_SPI_MASTER_FLAG				((uint32_t) 0x10000)
#define TCH_SPI_BUSY_FLAG				((uint32_t) 0x20000)

#define TCH_SPI_EVENT_RX_COMPLETE		((uint32_t) 0x01)
#define TCH_SPI_EVENT_TX_COMPLETE		((uint32_t) 0x02)
#define TCH_SPI_EVENT_OVR_ERR			((uint32_t) 0x04)
#define TCH_SPI_EVENT_UDR_ERR			((uint32_t) 0x08)
#define TCH_SPI_EVENT_ERR_MSK			(TCH_SPI_EVENT_UDR_ERR | TCH_SPI_EVENT_OVR_ERR)

#define TCH_SPI_EVENT_ALL				(TCH_SPI_EVENT_RX_COMPLETE|\
										TCH_SPI_EVENT_TX_COMPLETE|\
										TCH_SPI_EVENT_ERR_MSK)



#define SET_SAFE_RETURN();				SAFE_RETURN:
#define RETURN_SAFE()					goto SAFE_RETURN




#define SPI_setBusy(ins)             do{\
	set_system_busy();\
	((tch_spi_handle_prototype*) ins)->status |= TCH_SPI_BUSY_FLAG;\
}while(0)

#define SPI_clrBusy(ins)             do{\
	((tch_spi_handle_prototype*) ins)->status &= ~TCH_SPI_BUSY_FLAG;\
	clear_system_busy();\
}while(0)

#define SPI_isBusy(ins)              ((tch_spi_handle_prototype*) ins)->status & TCH_SPI_BUSY_FLAG


__USER_API__ static void tch_spi_initConfig(spi_config_t* cfg);
__USER_API__ static tch_spiHandle* tch_spi_open(const tch* env,spi_t spi,spi_config_t* cfg,uint32_t timeout,tch_PwrOpt popt);

__USER_API__ static tchStatus tch_spi_write(tch_spiHandle* self,const void* wb,size_t sz);
__USER_API__ static tchStatus tch_spi_read(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout);
__USER_API__ static tchStatus tch_spi_transceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
__USER_API__ static tchStatus tch_spi_transceiveDma(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
__USER_API__ static tchStatus tch_spi_close(tch_spiHandle* self);

static int tch_spi_init(void);
static void tch_spi_exit(void);
static BOOL tch_spi_handleInterrupt(tch_spi_handle_prototype* ins,tch_spi_descriptor* spiDesc);
static void tch_spi_validate(tch_spi_handle_prototype* ins);
static BOOL tch_spi_isValid(tch_spi_handle_prototype* ins);
static void tch_spi_invalidate(tch_spi_handle_prototype* ins);


struct tch_spi_request_s {
	int32_t        rsz;
	int32_t        tsz;
	uint8_t*       rxb;
	uint8_t*       txb;
	uint32_t       align;
};

struct tch_spi_handle_prototype {
	tch_spiHandle             pix;
	spi_t                     spi;
	const tch*                env;
	union {
		tch_dmaHandle        dma;
	}txCh;
	union {
		tch_dmaHandle        dma;
	}rxCh;
	tch_eventId               evId;
	uint32_t                  status;
	tch_gpioHandle*           iohandle;
	tch_mtxId                 mtx;
	tch_condvId               condv;
	tch_spi_request*          req;
};


__USER_RODATA__ tch_device_service_spi SPI_Ops = {
		.count = MFEATURE_SPI,
		.initCfg = tch_spi_initConfig,
		.allocSpi = tch_spi_open
};

static tch_mtxCb 	lock;
static tch_condvCb  condv;
static tch_device_service_dma* dma;

static int tch_spi_init(void)
{
	dma = NULL;
	tch_mutexInit(&lock);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_SPI,SPI_CLASS_KEY,&SPI_Ops,FALSE);
}

static void tch_spi_exit(void)
{
	tch_mutexDeinit(&lock);
	tch_condvDeinit(&condv);
	tch_kmod_unregister(MODULE_TYPE_SPI,SPI_CLASS_KEY);
}

MODULE_INIT(tch_spi_init);
MODULE_EXIT(tch_spi_exit);

__USER_API__ static void tch_spi_initConfig(spi_config_t* cfg)
{
	cfg->Baudrate = SPI_BAUDRATE_NORMAL;
	cfg->ClkMode = SPI_CLKMODE_0;
	cfg->FrmFormat = SPI_FRM_FORMAT_8B;
	cfg->FrmOrient = SPI_FRM_ORI_MSBFIRST;
	cfg->Role = SPI_ROLE_MASTER;
}

__USER_API__ static tch_spiHandle* tch_spi_open(const tch* env,spi_t spi,spi_config_t* cfg,uint32_t timeout,tch_PwrOpt popt)
{

	tch_spi_bs_t* spibs =  &SPI_BD_CFGs[spi];
	tch_spi_descriptor* spiDesc = &SPI_HWs[spi];
	tch_spi_handle_prototype* ins = NULL;
	SPI_TypeDef* spiHw = NULL;
	gpio_config_t iocfg;
	tch_DmaCfg dmaCfg;
	if(!dma)
		dma = tch_kmod_request(MODULE_TYPE_DMA);

	if(env->Mtx->lock(&lock,timeout) != tchOK)
		return NULL;
	while(spiDesc->_handle){
		if(env->Condv->wait(&lock,&condv,timeout) != tchOK)
			return NULL;
	}

	tch_device_service_gpio* gpio = (tch_device_service_gpio*) tch_kmod_request(MODULE_TYPE_GPIO);
	if(!gpio)
		return NULL;

	spiDesc->_handle = ins = env->Mem->alloc(sizeof(tch_spi_handle_prototype));
	if(env->Mtx->unlock(&lock) != tchOK)
	{
		return NULL;
	}
	mset(ins,0,sizeof(tch_spi_handle_prototype));
	iocfg.Af = spibs->afv;
	iocfg.Speed = GPIO_OSpeed_100M;
	iocfg.Mode = GPIO_Mode_AF;
	iocfg.popt = popt;

	gpio->allocIo(env,spibs->port,((1 << spibs->miso) | (1 << spibs->mosi) | (1 << spibs->sck)),&iocfg,timeout);

	if((spibs->rxdma != DMA_NOT_USED) && (spibs->txdma != DMA_NOT_USED) && dma)
	{
		dmaCfg.BufferType = DMA_BufferMode_Normal;
		dmaCfg.Ch = spibs->rxch;
		dmaCfg.Dir = DMA_Dir_PeriphToMem;
		dmaCfg.FlowCtrl = DMA_FlowControl_DMA;
		dmaCfg.Priority = DMA_Prior_Mid;
		if(cfg->FrmFormat == SPI_FRM_FORMAT_8B)
		{
			dmaCfg.mAlign = DMA_DataAlign_Byte;
			dmaCfg.pAlign = DMA_DataAlign_Byte;
		}
		else
		{
			dmaCfg.mAlign = DMA_DataAlign_Hword;
			dmaCfg.pAlign = DMA_DataAlign_Hword;
		}
		dmaCfg.mBurstSize = DMA_Burst_Single;
		dmaCfg.mInc = TRUE;
		dmaCfg.pBurstSize = DMA_Burst_Single;
		dmaCfg.pInc = FALSE;
		ins->rxCh.dma = dma->allocate(env,spibs->rxdma,&dmaCfg,timeout,popt);

		dmaCfg.Ch = spibs->txch;
		dmaCfg.Dir = DMA_Dir_MemToPeriph;
		ins->txCh.dma = dma->allocate(env,spibs->txdma,&dmaCfg,timeout,popt);
	}

	if((!ins->rxCh.dma || !ins->txCh.dma) && dma )
	{
		dma->freeDma(ins->rxCh.dma);
		dma->freeDma(ins->txCh.dma);
	}

	// all required resources are obtained successfully

	ins->condv = env->Condv->create();
	ins->mtx = env->Mtx->create();
	ins->evId = env->Event->create();
	ins->pix.read = tch_spi_read;
	ins->pix.write = tch_spi_write;
	ins->pix.close = tch_spi_close;
	ins->req = NULL;
	if(ins->rxCh.dma && ins->txCh.dma)
		ins->pix.transceive = tch_spi_transceiveDma;
	else
		ins->pix.transceive = tch_spi_transceive;
	ins->env = env;
	ins->spi = spi;


	// Initialize SPI registers
	spiHw = (SPI_TypeDef*) spiDesc->_hw;

	*spiDesc->_clkenr |= spiDesc->clkmsk;
	if(popt == ActOnSleep)
		*spiDesc->_lpclkenr |= spiDesc->lpclkmsk;

	*spiDesc->_rstr |= spiDesc->rstmsk;
	*spiDesc->_rstr &= ~spiDesc->rstmsk;


	spiHw->CR1 = 0;
	spiHw->CR1 |= ((cfg->FrmFormat << 11) | (cfg->FrmOrient << 7));

	if(cfg->ClkMode & 2)
		spiHw->CR1 |= SPI_CR1_CPOL;
	if(cfg->ClkMode & 1)
		spiHw->CR1 |= SPI_CR1_CPHA;

	if(cfg->Role == SPI_ROLE_MASTER)
	{
		spiHw->CR1 &= ~SPI_CR1_BR;
		spiHw->CR1 |= (cfg->Baudrate << 3);
		spiHw->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
		spiHw->CR1 |= SPI_CR1_MSTR;
	}
	else
	{
		spiHw->CR1 |= SPI_CR1_SSM;
		spiHw->CR1 &= ~SPI_CR1_SSI;
		spiHw->CR1 &= ~SPI_CR1_MSTR;
	}

	if(ins->txCh.dma && ins->rxCh.dma)
	{
		spiHw->CR2 |= (SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	}
	else
	{
		spiHw->CR2 |= SPI_CR2_ERRIE;
		tch_enableInterrupt(spiDesc->irq,HANDLER_NORMAL_PRIOR);
	}
	tch_spi_validate((tch_spi_handle_prototype*) ins);
	return (tch_spiHandle*) ins;
}

__USER_API__ static tchStatus tch_spi_write(tch_spiHandle* self,const void* wb,size_t sz)
{
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	if(!tch_spi_isValid(ins))
		return tchErrorResource;
	return ins->pix.transceive(self,wb,NULL,sz,tchWaitForever);
}

__USER_API__ static tchStatus tch_spi_read(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout)
{
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	if(!tch_spi_isValid(ins))
		return tchErrorResource;
	return ins->pix.transceive(self,NULL,rb,sz,timeout);
}

__USER_API__ static tchStatus tch_spi_transceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout)
{
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	void* twb = (void*) wb;
	if(!ins)
		return tchErrorParameter;
	if(!tch_spi_isValid(ins))
		return tchErrorResource;

	tchStatus result = tchOK;
	uint32_t sig = 0;
	tch_spi_descriptor* spiDesc = &SPI_HWs[ins->spi];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	const tch* env = ins->env;
	uint8_t offset = 1;
	if(spiHw->CR1 & SPI_CR1_DFF)
		offset = 2;

	if((result = env->Mtx->lock(ins->mtx,timeout)) != tchOK)
		return result;
	while(SPI_isBusy(ins)){
		if((result = env->Condv->wait(ins->condv,ins->mtx,timeout)) != tchOK)
			return result;
	}
	SPI_setBusy(ins);
	if((result = env->Mtx->unlock(ins->mtx)) != tchOK)
		return result;
	tch_spi_request req;
	ins->req = &req;

	req.align = offset;
	req.rxb = (uint8_t*) rb;
	req.txb = (uint8_t*) wb;
	req.tsz = req.rsz = sz;

	spiHw->CR1 |= SPI_CR1_SPE;
	spiHw->CR2 |= (SPI_CR2_TXEIE | SPI_CR2_RXNEIE);

	if((result = ins->env->Event->wait(ins->evId,(TCH_SPI_EVENT_RX_COMPLETE | TCH_SPI_EVENT_TX_COMPLETE),tchWaitForever)) != tchOK)
	{
		ins->env->Event->clear(ins->evId,TCH_SPI_EVENT_ALL);
		RETURN_SAFE();
	}

	if((sig = ins->env->Event->clear(ins->evId,TCH_SPI_EVENT_ALL)) & TCH_SPI_EVENT_ERR_MSK)
	{
		result =  tchErrorIo;
		RETURN_SAFE();
	}
	result = tchOK;

	SET_SAFE_RETURN();
	spiHw->CR2 &= ~(SPI_CR2_TXEIE | SPI_CR2_RXNEIE);
	spiHw->CR1 &= ~SPI_CR1_SPE;

	env->Mtx->lock(ins->mtx,tchWaitForever);
	ins->req = NULL;
	SPI_clrBusy(ins);
	env->Condv->wakeAll(ins->condv);
	env->Mtx->unlock(ins->mtx);
	return result;
}

__USER_API__ static tchStatus tch_spi_transceiveDma(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout)
{
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_spi_isValid(ins) || !dma)
		return tchErrorResource;
	tchStatus result = tchOK;
	tch_spi_descriptor* spiDesc = &SPI_HWs[ins->spi];
	SPI_TypeDef* spiHw = (SPI_TypeDef*) spiDesc->_hw;
	const tch* env = ins->env;
	tch_DmaReqDef dmaReq;

	if((result = env->Mtx->lock(ins->mtx,timeout)) != tchOK)
		return result;
	while(SPI_isBusy(ins)){
		if((result = env->Condv->wait(ins->condv,ins->mtx,timeout)) != tchOK)
			return result;
	}
	SPI_setBusy(ins);
	if((result = env->Mtx->unlock(ins->mtx)) != tchOK)
		return result;

	spiHw->CR1 |= SPI_CR1_SPE;
	dmaReq.MemAddr[0] = rb;
	if(rb)
		dmaReq.MemInc = TRUE;
	else
		dmaReq.MemInc = FALSE;
	dmaReq.PeriphAddr[0] = (uaddr_t)&spiHw->DR;
	dmaReq.PeriphInc = FALSE;
	dmaReq.size = sz;
	if(dma->beginXfer(ins->rxCh.dma,&dmaReq,0,&result)){
		result = tchErrorIo;
		RETURN_SAFE();
	}

	dmaReq.MemAddr[0] = (uaddr_t)wb;
	dmaReq.MemInc = TRUE;
	dmaReq.PeriphAddr[0] = (uaddr_t)&spiHw->DR;
	dmaReq.PeriphInc = FALSE;
	dmaReq.size = sz;
	if(dma->beginXfer(ins->txCh.dma,&dmaReq,timeout,&result)){
		result = tchErrorIo;
		RETURN_SAFE();
	}
	result = tchOK;
	SET_SAFE_RETURN();
	env->Mtx->lock(ins->mtx,tchWaitForever);
	spiHw->CR1 &= ~SPI_CR1_SPE;
	SPI_clrBusy(ins);
	env->Condv->wakeAll(ins->condv);
	env->Mtx->unlock(ins->mtx);
	return tchOK;
}


__USER_API__ static tchStatus tch_spi_close(tch_spiHandle* self)
{
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	tch_spi_descriptor* spiDesc = NULL;
	SPI_TypeDef* spiHw = NULL;
	if(!ins)
		return tchErrorParameter;
	if(!tch_spi_isValid(ins))
		return tchErrorResource;

	spiDesc = &SPI_HWs[ins->spi];
	spiHw = spiDesc->_hw;


	const tch* env = ins->env;
	tchStatus result = tchOK;
	env->Mtx->lock(ins->mtx,tchWaitForever);
	while(SPI_isBusy(ins))
	{
		env->Condv->wait(ins->condv,ins->mtx,tchWaitForever);
	}
	tch_spi_invalidate(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);
	env->Event->destroy(ins->evId);

	if(dma)
	{
		dma->freeDma(ins->rxCh.dma);
		dma->freeDma(ins->txCh.dma);
	}

	ins->iohandle->close(ins->iohandle);
	env->Mtx->lock(&lock,tchWaitForever);

	*spiDesc->_rstr |= spiDesc->rstmsk;
	*spiDesc->_clkenr &= ~spiDesc->clkmsk;
	*spiDesc->_lpclkenr &= ~spiDesc->lpclkmsk;
	tch_disableInterrupt(spiDesc->irq);

	spiDesc->_handle = NULL;
	env->Condv->wakeAll(&condv);
	env->Mtx->unlock(&lock);
	env->Mem->free(ins);
	return tchOK;

}

static void tch_spi_validate(tch_spi_handle_prototype* ins)
{
	ins->status &= ~0xFFFF;
	ins->status |= ((uint32_t)ins & 0xFFFF) ^ SPI_CLASS_KEY;
}

static BOOL tch_spi_isValid(tch_spi_handle_prototype* ins)
{
	return ((ins->status & 0xFFFF) == (((uint32_t) ins & 0xFFFF) ^ SPI_CLASS_KEY));
}

static void tch_spi_invalidate(tch_spi_handle_prototype* ins)
{
	ins->status &= ~0xFFFF;
}

static BOOL tch_spi_handleInterrupt(tch_spi_handle_prototype* ins,tch_spi_descriptor* spiDesc)
{
	SPI_TypeDef* spiHw = spiDesc->_hw;
	uint16_t readout;
	const tch* env = ins->env;
	tch_spi_request* req = ins->req;
	uint16_t sr = spiHw->SR;
	if(!spiDesc->_handle)
		return FALSE;
	if(sr & SPI_SR_TXE)
	{
		if(req)
		{
			if(req->tsz-- > 0)
			{
				spiHw->DR = *((uint16_t*) req->txb);
				req->txb += req->align;
				if(!req->tsz)
				{
					ins->env->Event->set(ins->evId,TCH_SPI_EVENT_TX_COMPLETE);
				}
			}
			else
			{
				if(req->rsz > 0)
					spiHw->DR = 0;
			}
		}
		else
		{
			return FALSE;
		}
	}
	if(sr & SPI_SR_RXNE)
	{
		if(req)
		{
			if(req->rsz-- > 0)
			{
				*((uint16_t*)req->rxb) = spiHw->DR;
				req->rxb += req->align;
				if(!req->rsz)
				{
					ins->env->Event->set(ins->evId,TCH_SPI_EVENT_RX_COMPLETE);
					spiHw->CR2 &= ~(SPI_CR2_RXNEIE | SPI_CR2_TXEIE);
				}
			}
		}
		else
		{
			return FALSE;
		}
	}
	if(sr & SPI_SR_OVR)
	{
		readout = spiHw->DR;
		ins->env->Event->set(ins->evId,TCH_SPI_EVENT_OVR_ERR);
		return TRUE;
	}
	if(sr & SPI_SR_UDR)
	{
		ins->env->Event->set(ins->evId,TCH_SPI_EVENT_UDR_ERR);
		return TRUE;
	}
	return FALSE;
}

void SPI1_IRQHandler(void)
{
	tch_spi_descriptor* spiDesc = &SPI_HWs[0];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);
}

void SPI2_IRQHandler(void)
{
	tch_spi_descriptor* spiDesc = &SPI_HWs[1];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);
}

void SPI3_IRQHandler(void)
{
	tch_spi_descriptor* spiDesc = &SPI_HWs[2];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);

}
