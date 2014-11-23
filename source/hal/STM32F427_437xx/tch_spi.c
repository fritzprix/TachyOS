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


#include "hal/tch_hal.h"
#include "tch_spi.h"
#include "tch_halInit.h"
#include "tch_halcfg.h"


typedef struct _tch_lld_spi_prototype tch_lld_spi_prototype;
typedef struct _tch_spi_handle_prototype tch_spi_handle_prototype;

#define SPI_ERR_MSK                  ((uint32_t) 0xF0000000)     // SPI Error Msk
#define SPI_ERR_OVR                  ((uint32_t) 0x10000000)     // SPI Overrun Error
#define SPI_ERR_UDR                  ((uint32_t) 0x20000000)     // SPI Underrun Error


#define TCH_SPI_CLASS_KEY            ((uint16_t) 0x41F5)
#define TCH_SPI_MASTER_FLAG          ((uint32_t) 0x10000)
#define TCH_SPI_BUSY_FLAG            ((uint32_t) 0x20000)

#define SPI_FRM_FORMAT_16B           ((uint8_t) 1)
#define SPI_FRM_FORMAT_8B            ((uint8_t) 0)

#define SPI_FRM_ORI_MSBFIRST         ((uint8_t) 0)
#define SPI_FRM_ORI_LSBFIRST         ((uint8_t) 1)

#define SPI_OPMODE_MASTER            ((uint8_t) 0)
#define SPI_OPMODE_SLAVE             ((uint8_t) 1)

#define SPI_CLKMODE_0                ((uint8_t) 0)
#define SPI_CLKMODE_1                ((uint8_t) 1)
#define SPI_CLKMODE_2                ((uint8_t) 2)
#define SPI_CLKMODE_3                ((uint8_t) 3)

#define SPI_BAUDRATE_HIGH            ((uint8_t) 0)
#define SPI_BAUDRATE_NORMAL          ((uint8_t) 2)
#define SPI_BAUDRATE_LOW             ((uint8_t) 4)


#define SPI_setBusy(ins)             do{\
	((tch_spi_handle_prototype*) ins)->status |= TCH_SPI_BUSY_FLAG;\
}while(0)

#define SPI_clrBusy(ins)             do{\
	((tch_spi_handle_prototype*) ins)->status &= ~TCH_SPI_BUSY_FLAG;\
}while(0)

#define SPI_isBusy(ins)              ((tch_spi_handle_prototype*) ins)->status & TCH_SPI_BUSY_FLAG





#define INIT_SPI_STR                 {0,1,2,3,4,5,6,7,8}
#define INIT_SPI_FRM_FORMAT          {\
	                                   SPI_FRM_FORMAT_16B,\
	                                   SPI_FRM_FORMAT_8B\
}

#define INIT_SPI_FRM_ORIENT          {\
	                                   SPI_FRM_ORI_MSBFIRST,\
	                                   SPI_FRM_ORI_LSBFIRST\
}

#define INIT_SPI_OPMODE              {\
	                                   SPI_OPMODE_MASTER,\
	                                   SPI_OPMODE_SLAVE\
}

#define INIT_SPI_CLKMODE             {\
	                                   SPI_CLKMODE_0,\
	                                   SPI_CLKMODE_1,\
	                                   SPI_CLKMODE_2,\
	                                   SPI_CLKMODE_3\
}

#define INIT_SPI_BUADRATE            {\
                                       SPI_BAUDRATE_HIGH,\
                                       SPI_BAUDRATE_NORMAL,\
                                       SPI_BAUDRATE_LOW\
}


static void tch_spiInitCfg(tch_spiCfg* cfg);
static tch_spiHandle* tch_spiOpen(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout,tch_PwrOpt popt);

static tchStatus tch_spiWrite(tch_spiHandle* self,const void* wb,size_t sz);
static tchStatus tch_spiRead(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout);
static tchStatus tch_spiTransceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
static tchStatus tch_spiTransceiveDma(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
static tchStatus tch_spiClose(tch_spiHandle* self);

static BOOL tch_spi_handleInterrupt(tch_spi_handle_prototype* ins,tch_spi_descriptor* spiDesc);
static void tch_spiValidate(tch_spi_handle_prototype* ins);
static BOOL tch_spiIsValid(tch_spi_handle_prototype* ins);
static void tch_spiInvalidate(tch_spi_handle_prototype* ins);


struct _tch_lld_spi_prototype {
	tch_lld_spi               pix;
	tch_mtxId                 mtx;
	tch_condvId               condv;
};

struct _tch_spi_handle_prototype {
	tch_spiHandle             pix;
	spi_t                     spi;
	const tch*                env;
	union {
		tch_DmaHandle        dma;
		tch_msgqId           mq;
	}txCh;
	union {
		tch_DmaHandle        dma;
		tch_msgqId           mq;
	}rxCh;
	uint32_t                  status;
	tch_GpioHandle*           iohandle;
	tch_mtxId                 mtx;
	tch_condvId               condv;
};

/**
 */
__attribute__((section(".data"))) static tch_lld_spi_prototype SPI_StaticInstance = {
		{
			INIT_SPI_STR,
			INIT_SPI_CLKMODE,
			INIT_SPI_FRM_FORMAT,
			INIT_SPI_FRM_ORIENT,
			INIT_SPI_OPMODE,
			INIT_SPI_BUADRATE,
			tch_spiInitCfg,
			tch_spiOpen
		},
		NULL,
		NULL

};

const tch_lld_spi* tch_spi_instance = (const tch_lld_spi*) &SPI_StaticInstance;



static void tch_spiInitCfg(tch_spiCfg* cfg){
	cfg->Baudrate = SPI_BAUDRATE_NORMAL;
	cfg->ClkMode = SPI_CLKMODE_0;
	cfg->FrmFormat = SPI_FRM_FORMAT_8B;
	cfg->FrmOrient = SPI_FRM_ORI_MSBFIRST;
	cfg->Role = SPI_OPMODE_MASTER;
}

static tch_spiHandle* tch_spiOpen(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout,tch_PwrOpt popt){

	tch_spi_bs* spibs =  &SPI_BD_CFGs[spi];
	tch_spi_descriptor* spiDesc = &SPI_HWs[spi];
	tch_spi_handle_prototype* hnd = NULL;
	SPI_TypeDef* spiHw = NULL;
	tch_GpioCfg iocfg;
	tch_DmaCfg dmaCfg;
	BOOL rxDma = FALSE;
	BOOL txDma = FALSE;

	if(!SPI_StaticInstance.mtx)
		SPI_StaticInstance.mtx = env->Mtx->create();
	if(!SPI_StaticInstance.condv)
		SPI_StaticInstance.condv = env->Condv->create();


	if(env->Mtx->lock(SPI_StaticInstance.mtx,timeout) != osOK)
		return NULL;
	while(spiDesc->_handle){
		if(env->Condv->wait(SPI_StaticInstance.mtx,SPI_StaticInstance.condv,timeout) != osOK)
			return NULL;
	}

	spiDesc->_handle = hnd = env->Mem->alloc(sizeof(tch_spi_handle_prototype));
	if(env->Mtx->unlock(SPI_StaticInstance.mtx) != osOK){
		return NULL;
	}

	env->uStdLib->string->memset(hnd,0,sizeof(tch_spi_handle_prototype));

	iocfg.Af = spibs->afv;
	iocfg.Speed = env->Device->gpio->Speed.High;
	iocfg.Mode = env->Device->gpio->Mode.Func;

	hnd->iohandle = env->Device->gpio->allocIo(env,spibs->port,((1 << spibs->miso) | (1 << spibs->mosi) | (1 << spibs->sck)),&iocfg,timeout,popt);

	if((spibs->rxdma != DMA_NOT_USED) && (spibs->txdma != DMA_NOT_USED)){
		dmaCfg.BufferType = env->Device->dma->BufferType.Normal;
		dmaCfg.Ch = spibs->rxch;
		dmaCfg.Dir = env->Device->dma->Dir.PeriphToMem;
		dmaCfg.FlowCtrl = env->Device->dma->FlowCtrl.DMA;
		dmaCfg.Priority = env->Device->dma->Priority.Normal;
		if(cfg->FrmFormat == SPI_FRM_FORMAT_8B){
			dmaCfg.mAlign = env->Device->dma->Align.Byte;
			dmaCfg.pAlign = env->Device->dma->Align.Byte;
		}
		else{
			dmaCfg.mAlign = env->Device->dma->Align.word;
			dmaCfg.pAlign = env->Device->dma->Align.Byte;
		}
		dmaCfg.mBurstSize = env->Device->dma->BurstSize.Burst1;
		dmaCfg.mInc = TRUE;
		dmaCfg.pBurstSize = env->Device->dma->BurstSize.Burst1;
		dmaCfg.pInc = FALSE;
		hnd->rxCh.dma = env->Device->dma->allocDma(env,spibs->rxdma,&dmaCfg,timeout,popt);
		if(hnd->rxCh.dma)
			rxDma = TRUE;

		dmaCfg.Ch = spibs->txch;
		dmaCfg.Dir = env->Device->dma->Dir.MemToPeriph;
		hnd->txCh.dma = env->Device->dma->allocDma(env,spibs->txdma,&dmaCfg,timeout,popt);
		if(hnd->txCh.dma)
			txDma = TRUE;
	}

	if(!rxDma || !txDma){
		env->Device->dma->freeDma(hnd->rxCh.dma);
		env->Device->dma->freeDma(hnd->txCh.dma);
		hnd->rxCh.mq = env->MsgQ->create(1);
		hnd->txCh.mq = env->MsgQ->create(1);
	}

	if(!hnd->txCh.dma || !hnd->rxCh.dma){
		env->Mtx->lock(SPI_StaticInstance.mtx,osWaitForever);
		spiDesc->_handle = NULL;
		env->Condv->wakeAll(SPI_StaticInstance.condv);
		env->Mtx->unlock(SPI_StaticInstance.mtx);
		if(hnd->iohandle)
			hnd->iohandle->close(hnd->iohandle);
		if(hnd->rxCh.dma){
			if(rxDma)
				env->Device->dma->freeDma(hnd->rxCh.dma);
			else
				env->MsgQ->destroy(hnd->rxCh.mq);
		}
		if(hnd->txCh.dma){
			if(txDma)
				env->Device->dma->freeDma(hnd->txCh.dma);
			else
				env->MsgQ->destroy(hnd->txCh.mq);
		}
		env->Mem->free(hnd);
		return NULL;
	}

	// all required resources are obtained successfully

	hnd->condv = env->Condv->create();
	hnd->mtx = env->Mtx->create();
	hnd->pix.read = tch_spiRead;
	hnd->pix.write = tch_spiWrite;
	hnd->pix.close = tch_spiClose;
	if(rxDma && txDma)
		hnd->pix.transceive = tch_spiTransceiveDma;
	else
		hnd->pix.transceive = tch_spiTransceive;
	hnd->env = env;
	hnd->spi = spi;


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

	if(cfg->Role == SPI_OPMODE_MASTER){
		spiHw->CR1 &= ~SPI_CR1_BR;
		spiHw->CR1 |= (cfg->Baudrate << 3);
		spiHw->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
		spiHw->CR1 |= SPI_CR1_MSTR;
	}else{
		spiHw->CR1 |= SPI_CR1_SSM;
		spiHw->CR1 &= ~SPI_CR1_SSI;
		spiHw->CR1 &= ~SPI_CR1_MSTR;
	}
	if(txDma && rxDma)
		spiHw->CR2 |= (SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
	else{
		spiHw->CR2 |= (SPI_CR2_RXNEIE | SPI_CR2_ERRIE);
		env->Device->interrupt->setPriority(spiDesc->irq,env->Device->interrupt->Priority.Normal);
		env->Device->interrupt->enable(spiDesc->irq);
		__DMB();
		__ISB();
	}

	spiHw->CR1 |= SPI_CR1_SPE;

	tch_spiValidate((tch_spi_handle_prototype*) hnd);
	SPI_clrBusy(hnd);
	return (tch_spiHandle*) hnd;
}

static tchStatus tch_spiWrite(tch_spiHandle* self,const void* wb,size_t sz){
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	if(!tch_spiIsValid(ins))
		return osErrorResource;
	return ins->pix.transceive(self,wb,NULL,sz,osWaitForever);
}

static tchStatus tch_spiRead(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout){
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	if(!tch_spiIsValid(ins))
		return osErrorResource;
	return ins->pix.transceive(self,NULL,rb,sz,timeout);
}

static tchStatus tch_spiTransceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout){
	tch_spi_handle_prototype* hnd = (tch_spi_handle_prototype*) self;
	void* twb = (void*) wb;
	if(!hnd)
		return osErrorParameter;
	if(!tch_spiIsValid(hnd))
		return osErrorResource;
	osEvent evt;


	tch_spi_descriptor* spiDesc = &SPI_HWs[hnd->spi];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	const tch* env = hnd->env;
	uint8_t offset = 1;
	if(spiHw->CR1 & SPI_CR1_DFF)
		offset = 2;

	if((evt.status = env->Mtx->lock(hnd->mtx,timeout)) != osOK)
		return evt.status;
	while(SPI_isBusy(hnd)){
		if((evt.status = env->Condv->wait(hnd->condv,hnd->mtx,timeout)) != osOK)
			return evt.status;
	}
	SPI_setBusy(hnd);
	if((evt.status = env->Mtx->unlock(hnd->mtx)) != osOK)
		return evt.status;

	while(sz--){
		spiHw->DR = *((uint8_t*)twb);  // load data to tx buffer
		if(twb)                           // if wb is null, address isn't incremented
			twb += offset;
		evt = hnd->env->MsgQ->get(hnd->rxCh.mq,timeout);   // get rx data
		if(evt.status != osEventMessage){                  // check message has been received successfully
			return evt.status;
		}
		if(evt.value.v & SPI_ERR_MSK)
			return osErrorValue;
		if(offset == 1){
			*(uint8_t*) rb = evt.value.v;                    // if message is obtained successfully, save it to rx buffer
		}else if(offset == 2){
			*(uint16_t*) rb = evt.value.v;
		}
		if(rb)                            // if rb is null, address isn't incremented
			rb += offset;
	}
	if((evt.status = env->Mtx->lock(hnd->mtx,osWaitForever)) != osOK)
		return evt.status;
	SPI_clrBusy(hnd);
	evt.status = env->Condv->wakeAll(hnd->condv);
	env->Mtx->unlock(hnd->mtx);
	return osOK;
}

static tchStatus tch_spiTransceiveDma(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout){
	tch_spi_handle_prototype* hnd = (tch_spi_handle_prototype*) self;
	if(!hnd)
		return osErrorParameter;
	if(!tch_spiIsValid(hnd))
		return osErrorResource;
	tchStatus result = osOK;
	tch_spi_descriptor* spiDesc = &SPI_HWs[hnd->spi];
	SPI_TypeDef* spiHw = (SPI_TypeDef*) spiDesc->_hw;
	const tch* env = hnd->env;
	tch_DmaReqDef dmaReq;

	if((result = env->Mtx->lock(hnd->mtx,timeout)) != osOK)
		return result;
	while(SPI_isBusy(hnd)){
		if((result = env->Condv->wait(hnd->condv,hnd->mtx,timeout)) != osOK)
			return result;
	}
	SPI_setBusy(hnd);
	if((result = env->Mtx->unlock(hnd->mtx)) != osOK)
		return result;
	dmaReq.MemAddr[0] = rb;
	if(rb)
		dmaReq.MemInc = TRUE;
	else
		dmaReq.MemInc = FALSE;
	dmaReq.PeriphAddr[0] = (uaddr_t)&spiHw->DR;
	dmaReq.PeriphInc = FALSE;
	dmaReq.size = sz;
	env->Device->dma->beginXfer(hnd->rxCh.dma,&dmaReq,0,&result);

	dmaReq.MemAddr[0] = (uaddr_t)wb;
	dmaReq.MemInc = TRUE;
	dmaReq.PeriphAddr[0] = (uaddr_t)&spiHw->DR;
	dmaReq.PeriphInc = FALSE;
	dmaReq.size = sz;
	result = env->Device->dma->beginXfer(hnd->txCh.dma,&dmaReq,timeout,&result);

	if((result = env->Mtx->lock(hnd->mtx,osWaitForever)) != osOK)
		return result;
	SPI_clrBusy(hnd);
	env->Condv->wakeAll(hnd->condv);
	if((result = env->Mtx->unlock(hnd->mtx)) != osOK)
		return result;
	return osOK;
}


static tchStatus tch_spiClose(tch_spiHandle* self){
	tch_spi_handle_prototype* ins = (tch_spi_handle_prototype*) self;
	tch_spi_descriptor* spiDesc = NULL;
	SPI_TypeDef* spiHw = NULL;
	if(!ins)
		return osErrorParameter;
	if(!tch_spiIsValid(ins))
		return osErrorResource;

	spiDesc = &SPI_HWs[ins->spi];
	spiHw = spiDesc->_hw;


	const tch* env = ins->env;
	tchStatus result = osOK;
	env->Mtx->lock(ins->mtx,osWaitForever);
	while(SPI_isBusy(ins)){
		env->Condv->wait(ins->condv,ins->mtx,osWaitForever);
	}
	tch_spiInvalidate(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);

	env->Device->dma->freeDma(ins->rxCh.dma);
	env->Device->dma->freeDma(ins->txCh.dma);
	env->MsgQ->destroy(ins->rxCh.mq);
	env->MsgQ->destroy(ins->txCh.mq);

	ins->iohandle->close(ins->iohandle);

	env->Mtx->lock(SPI_StaticInstance.mtx,osWaitForever);

	*spiDesc->_rstr |= spiDesc->rstmsk;
	*spiDesc->_rstr &= ~spiDesc->rstmsk;

	*spiDesc->_clkenr &= ~spiDesc->clkmsk;
	*spiDesc->_lpclkenr &= ~spiDesc->lpclkmsk;

	spiDesc->_handle = NULL;
	env->Condv->wakeAll(SPI_StaticInstance.condv);
	env->Mtx->unlock(SPI_StaticInstance.mtx);

	env->Mem->free(ins);

	return osOK;

}

static void tch_spiValidate(tch_spi_handle_prototype* ins){
	ins->status &= ~0xFFFF;
	ins->status |= ((uint32_t)ins & 0xFFFF) ^ TCH_SPI_CLASS_KEY;
}

static BOOL tch_spiIsValid(tch_spi_handle_prototype* ins){
	return ((ins->status & 0xFFFF) == (((uint32_t) ins & 0xFFFF) ^ TCH_SPI_CLASS_KEY));
}

static void tch_spiInvalidate(tch_spi_handle_prototype* ins){
	ins->status &= ~0xFFFF;
}

static BOOL tch_spi_handleInterrupt(tch_spi_handle_prototype* ins,tch_spi_descriptor* spiDesc){
	SPI_TypeDef* spiHw = spiDesc->_hw;
	const tch* env = ins->env;
	if(!spiDesc->_handle)
		return FALSE;
	if(spiHw->SR & SPI_SR_RXNE){
		env->MsgQ->put(ins->rxCh.mq,spiHw->DR,0);
		return TRUE;
	}
	if(spiHw->SR & SPI_SR_OVR){
		env->MsgQ->put(ins->rxCh.mq,spiHw->DR,0);   // read data and ovr flag will be cleared
		return TRUE;
	}
	if(spiHw->SR & SPI_SR_UDR){
		env->MsgQ->put(ins->rxCh.mq,SPI_ERR_UDR,0); // send error type as msg
		return TRUE;
	}
	return FALSE;
}

void SPI1_IRQHandler(void){
	tch_spi_descriptor* spiDesc = &SPI_HWs[0];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);
}

void SPI2_IRQHandler(void){
	tch_spi_descriptor* spiDesc = &SPI_HWs[1];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);
}

void SPI3_IRQHandler(void){
	tch_spi_descriptor* spiDesc = &SPI_HWs[2];
	SPI_TypeDef* spiHw = spiDesc->_hw;
	if(!tch_spi_handleInterrupt(spiDesc->_handle,spiDesc))
		spiHw->SR &= ~(spiHw->SR);

}
