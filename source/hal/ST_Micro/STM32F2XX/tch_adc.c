/*
 * tch_adc.c
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
#include "tch_adc.h"
#include "tch_kernel.h"

#define SET_SAFE_EXIT();      RETURN_EXIT:
#define RETURN_SAFE()         goto RETURN_EXIT
#define _1_MHZ                ((uint32_t) 1000000)
#define ADC_Precision_Pos     ((uint8_t) 24)
#define ADC_CR2_EXTSEL_Pos    ((uint8_t) 24)

#define ADC_CLASS_KEY         ((uint16_t) 0x4230)
#define ADC_BUSY_FLAG         ((uint32_t) 0x10000)


#define ADC_setBusy(ins)             do{\
	((tch_adc_handle_prototype*) ins)->status |= ADC_BUSY_FLAG;\
	tch_kernelSetBusyMark();\
}while(0)

#define ADC_clrBusy(ins)             do{\
	((tch_adc_handle_prototype*) ins)->status &= ~ADC_BUSY_FLAG;\
	tch_kernelClrBusyMark();\
}while(0)

#define ADC_isBusy(ins)              ((tch_adc_handle_prototype*) ins)->status & ADC_BUSY_FLAG



typedef struct tch_lld_adc_prototype {
	tch_lld_adc                          pix;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
}tch_lld_adc_prototype;


typedef struct tch_adc_handle_prototype {
	tch_adcHandle                        pix;
	const tch*                           env;
	uint32_t                             status;
	uint16_t                             isr_msk;
	uint32_t                             ch_occp;
	adc_t                                adc;
	tch_DmaHandle                        dma;
	tch_pwmHandle*                       timer;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
	uint8_t                              chcnt;
	tch_GpioHandle**                     io_handle;
	tch_msgqId                           msgq;
}tch_adc_handle_prototype;



static tch_adcHandle* tch_adcOpen(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout);
static tchStatus tch_adcClose(tch_adcHandle* self);
static uint32_t tch_adcRead(const tch_adcHandle* self,uint8_t ch);
static tchStatus tch_adcBurstConvert(const tch_adcHandle* self,uint8_t ch,tch_mailqId q,uint32_t convCnt);
static uint32_t tch_adcChannelOccpStatus;


static void tch_adcCfgInit(tch_adcCfg* cfg);
static void tch_adcAddChannel(tch_adcChDef* chdef,uint8_t ch);
static void tch_adcRemoveChannel(tch_adcChDef* chdef,uint8_t ch);
static BOOL tch_adcAvailable(adc_t adc);

static void tch_adcValidate(tch_adc_handle_prototype* ins);
static void tch_adcInvalidata(tch_adc_handle_prototype* ins);
static BOOL tch_adcIsValid(tch_adc_handle_prototype* ins);
static BOOL tch_adcHandleInterrupt(tch_adc_descriptor* adcDesc,tch_adc_handle_prototype* ins);





static void tch_adc_setRegChannel(tch_adc_descriptor* ins,uint8_t ch,uint8_t order);
static void tch_adc_setRegSampleHold(tch_adc_descriptor* ins,uint8_t ch,uint8_t ADC_SampleHold);



static tch_lld_adc_prototype ADC_StaticInstance = {
		{
				MFEATURE_ADC,
				12,
				6,
				tch_adcCfgInit,
				tch_adcAddChannel,
				tch_adcRemoveChannel,
				tch_adcOpen,
				tch_adcAvailable
		},
		NULL,
		NULL
};

tch_lld_adc* tch_adcHalInit(const tch* env){
	if(ADC_StaticInstance.mtx || ADC_StaticInstance.condv)
		return NULL;
	if(!env)
		return NULL;
	tch_adcChannelOccpStatus = 0;
	ADC_StaticInstance.mtx = env->Mtx->create();
	ADC_StaticInstance.condv = env->Condv->create();
	return (tch_lld_adc*) &ADC_StaticInstance;
}

static tch_adcHandle* tch_adcOpen(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout){
	tch_adc_handle_prototype* ins = NULL;
	uint8_t ch_idx = 0;
	tch_DmaCfg dmacfg;
	if(!ADC_StaticInstance.mtx)
		ADC_StaticInstance.mtx = env->Mtx->create();
	if(!ADC_StaticInstance.condv)
		ADC_StaticInstance.condv = env->Condv->create();
	if(adc > MFEATURE_ADC)
		return NULL;
	if(!(cfg->chdef.chselMsk > 0))
		return NULL;

	tch_adc_descriptor* adcDesc = &ADC_HWs[adc];
	tch_adc_bs* adcBs = &ADC_BD_CFGs[adc];
	tch_adc_channel_bs* adcChBs = NULL;
	ADC_TypeDef* adcHw = adcDesc->_hw;
	uint8_t smphld = 0;

	if(env->Mtx->lock(ADC_StaticInstance.mtx,timeout) != tchOK)
		return NULL;
	while(adcDesc->_handle && (cfg->chdef.chselMsk& tch_adcChannelOccpStatus)){
		if(env->Condv->wait(ADC_StaticInstance.condv,ADC_StaticInstance.mtx,timeout) != tchOK)
			return NULL;
	}
	ins = adcDesc->_handle = env->Mem->alloc(sizeof(tch_adc_handle_prototype));
	env->uStdLib->string->memset(ins,0,sizeof(tch_adc_handle_prototype));
	tch_adcChannelOccpStatus |= (ins->ch_occp = cfg->chdef.chselMsk);
	if(env->Mtx->unlock(ADC_StaticInstance.mtx) != tchOK)
		return NULL;

	ins->io_handle = (tch_GpioHandle**) env->Mem->alloc(sizeof(tch_GpioHandle*) * cfg->chdef.chcnt);
	ins->mtx = env->Mtx->create();
	ins->condv = env->Condv->create();
	ins->msgq = env->MsgQ->create(1);
	// gpio initialize
	tch_GpioCfg iocfg;
	env->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_AN;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.popt = popt;


	*adcDesc->_clkenr |= adcDesc->clkmsk;
	if(popt == ActOnSleep){
		*adcDesc->_lpclkenr |= adcDesc->lpclkmsk;
	}

	switch(cfg->SampleHold){
	case ADC_SampleHold_VeryLong:
		smphld = 7;
		break;
	case ADC_SampleHold_Long:
		smphld = 4;
		break;
	case ADC_SampleHold_Mid:
		smphld = 2;
		break;
	case ADC_SampleHold_Short:
		smphld = 0;
		break;
	}

	for(ch_idx = 0,ins->chcnt = 0;(cfg->chdef.chselMsk && (cfg->chdef.chcnt));cfg->chdef.chcnt--){
		if(cfg->chdef.chselMsk & 1){
			adcChBs = &ADC_CH_BD_CFGs[ch_idx];
			if(adcChBs->adc_routemsk & (1 << adc)){
				ins->io_handle[ins->chcnt++] = env->Device->gpio->allocIo(env,adcChBs->port.port,(1 << adcChBs->port.pin),&iocfg,timeout);
				tch_adc_setRegSampleHold(adcDesc,ch_idx,smphld);
				tch_adc_setRegChannel(adcDesc,ch_idx,0);
			}
		}
		cfg->chdef.chselMsk >>= 1;
		ch_idx++;
	}


	// dma initialize
	if(adcBs->dma != DMA_NOT_USED){
		/// initialize dma and configure relevant register of adc
		dmacfg.BufferType = DMA_BufferMode_Normal;
		dmacfg.Ch = adcBs->dmaCh;
		dmacfg.Dir = DMA_Dir_PeriphToMem;
		dmacfg.FlowCtrl = DMA_FlowControl_DMA;
		dmacfg.Priority = DMA_Prior_Mid;
		dmacfg.mAlign = DMA_DataAlign_Hword;
		dmacfg.mInc = TRUE;
		dmacfg.mBurstSize = DMA_Burst_Single;
		dmacfg.pAlign = DMA_DataAlign_Hword;
		dmacfg.pBurstSize = DMA_Burst_Single;
		dmacfg.pInc = TRUE;
		ins->dma = tch_dma->allocDma(env,adcBs->dma,&dmacfg,timeout,popt);
	}else{
		// handle dma_not_used
	}

	tch_pwmDef pwmDef;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = _1_MHZ / cfg->SampleFreq;
	pwmDef.pwrOpt = popt;
	ins->timer = env->Device->timer->openPWM(env,adcBs->timer,&pwmDef,timeout);

	ins->isr_msk |= ADC_SR_OVR | ADC_SR_EOC;
	adcHw->CR1 |= (cfg->Precision << ADC_Precision_Pos) | ADC_CR1_OVRIE | ADC_CR1_EOCIE;
	adcHw->CR2 |= (ADC_CR2_ADON | (adcBs->timerExtsel << ADC_CR2_EXTSEL_Pos));

	ins->pix.close = tch_adcClose;
	ins->pix.readBurst = tch_adcBurstConvert;
	ins->pix.read = tch_adcRead;
	ins->env = env;

	NVIC_SetPriority(adcDesc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(adcDesc->irq);

	tch_adcValidate(ins);
	return (tch_adcHandle*) ins;
}

static BOOL tch_adcAvailable(adc_t adc){
	if(adc >= MFEATURE_ADC)
		return FALSE;
	return ADC_HWs[adc]._handle == NULL;
}


static tchStatus tch_adcClose(tch_adcHandle* self){
	tch_adc_handle_prototype* ins =(tch_adc_handle_prototype*) self;
	tch_adc_descriptor* adcDesc = &ADC_HWs[ins->adc];
	tchStatus  result = tchOK;
	if(!self)
		return tchErrorParameter;
	if(!tch_adcIsValid(ins))
		return tchErrorParameter;
	const tch* env = ins->env;
	if((result = env->Mtx->lock(ins->mtx,osWaitForever)) != tchOK)
		return result;
	while(ADC_isBusy(ins)){
		if((result = env->Condv->wait(ins->condv,ins->mtx,osWaitForever)) != tchOK)
			return result;
	}
	tch_adcInvalidata(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);
	while(ins->chcnt--){
		ins->io_handle[ins->chcnt]->close(ins->io_handle[ins->chcnt]);
	}
	env->Mem->free(ins->io_handle);
	tch_dma->freeDma(ins->dma);
	ins->timer->close(ins->timer);

	if((result = env->Mtx->lock(ADC_StaticInstance.mtx,osWaitForever)) != tchOK)
		return result;
	adcDesc->_handle = NULL;
	tch_adcChannelOccpStatus &= ins->ch_occp;
	env->Condv->wakeAll(ADC_StaticInstance.condv);
	env->Mtx->unlock(ADC_StaticInstance.mtx);

	env->Mem->free(ins);
	return tchOK;
}

static uint32_t tch_adcRead(const tch_adcHandle* self,uint8_t ch){
	tch_adc_handle_prototype* ins = (tch_adc_handle_prototype*) self;
	tch_adc_descriptor* adcDesc = &ADC_HWs[ins->adc];
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	tchEvent evt;
	if(!ins)
		return ADC_READ_FAIL;
	if(!tch_adcIsValid(ins))
		return ADC_READ_FAIL;
	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != tchOK)
		return ADC_READ_FAIL;
	while(ADC_isBusy(ins)){
		if(ins->env->Condv->wait(ins->condv,ins->mtx,osWaitForever) != tchOK)
			return ADC_READ_FAIL;
	}
	ADC_setBusy(ins);
	if(ins->env->Mtx->unlock(ins->mtx) != tchOK)
		return ADC_READ_FAIL;
	tch_adc_setRegChannel(adcDesc,ch,1);
	adcHw->CR2 |= ADC_CR2_SWSTART;    /// start conversion
	evt = ins->env->MsgQ->get(ins->msgq,osWaitForever);

	if(evt.status != tchEventMessage){
		evt.value.v = ADC_READ_FAIL;
		RETURN_SAFE();
	}
	SET_SAFE_EXIT();
	ins->env->Mtx->lock(ins->mtx,osWaitForever);
	ADC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return evt.value.v;
}

static tchStatus tch_adcBurstConvert(const tch_adcHandle* self,uint8_t ch,tch_mailqId q,uint32_t convCnt){
	tch_adc_handle_prototype* ins = (tch_adc_handle_prototype*) self;
	ADC_TypeDef* adcHw = NULL;
	tch_adc_bs* adcBs = NULL;
	tchEvent evt;
	tchStatus result = tchOK;
	tch_adc_descriptor* adcDesc = NULL;
	if(!ins)
		return tchErrorParameter;
	if(!tch_adcIsValid(ins))
		return tchErrorParameter;

	adcBs = &ADC_BD_CFGs[ins->adc];
	adcDesc = &ADC_HWs[ins->adc];
	adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if((result = ins->env->Mtx->lock(ins->mtx,osWaitForever)) != tchOK)
		return result;
	while(ADC_isBusy(ins)){
		if((result = ins->env->Condv->wait(ins->condv,ins->mtx,osWaitForever)) != tchOK)
			return result;
	}
	ADC_setBusy(ins);
	if((result = ins->env->Mtx->unlock(ins->mtx)) != tchOK)
		return result;

	ins->timer->setDuty(ins->timer,adcBs->timerCh,0.5f);

	uint32_t chnksz = ins->env->MailQ->getBlockSize(q);

	tch_adc_setRegChannel(adcDesc,ch,1);
	//enable dma
	adcHw->CR2 |= ADC_CR2_DMA;
	adcHw->CR2 |= ADC_CR2_EXTEN_0;
	tch_DmaReqDef dmaReq;
	uint16_t* chnk = NULL;
	while(convCnt--){
		chnk = (uint16_t*) ins->env->MailQ->alloc(q,osWaitForever,NULL);
		tch_dma->initReq(&dmaReq,chnk,(uint32_t*) &adcHw->DR,(chnksz / 2));    // burst unit is half word
		ins->timer->start(ins->timer);
		tch_dma->beginXfer(ins->dma,&dmaReq,0,&result);
		evt = ins->env->MsgQ->get(ins->msgq,osWaitForever);
		ins->timer->stop(ins->timer);
		ins->env->MailQ->put(q,chnk);
		if(evt.status != tchEventMessage){
			evt.status = tchErrorIo;
			RETURN_SAFE();
		}
	}

	evt.status = tchOK;

	SET_SAFE_EXIT();
	adcHw->CR2 &= ~ADC_CR2_DMA;
	adcHw->CR2 &= ~ADC_CR2_EXTEN_0;
	ins->env->Mtx->lock(ins->mtx,osWaitForever);
	ADC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);

	return evt.status;

}



static void tch_adcCfgInit(tch_adcCfg* cfg){
	cfg->chdef.chcnt = cfg->chdef.chselMsk = 0;
	cfg->Precision = ADC_Precision_8B;
	cfg->SampleFreq = 1000;
	cfg->SampleHold = ADC_SampleHold_Mid;
}

static void tch_adcAddChannel(tch_adcChDef* chdef,uint8_t ch){
	chdef->chselMsk |= (1 << ch);
	chdef->chcnt++;
}

static void tch_adcRemoveChannel(tch_adcChDef* chdef,uint8_t ch){
	chdef->chselMsk &= ~(1 << ch);
	chdef->chcnt--;
}


static void tch_adc_setRegChannel(tch_adc_descriptor* adcDesc,uint8_t ch,uint8_t order){
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(order > 12){
		adcHw->SQR1 |= (ch << ((order - 13) * 4));
	}else if(order > 6){
		adcHw->SQR2 |= (ch << ((order - 7) * 4));
	}else{
		adcHw->SQR3 |= (ch << (order * 4));
	}
}


static void tch_adc_setRegSampleHold(tch_adc_descriptor* adcDesc,uint8_t ch,uint8_t ADC_SampleHold){
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(ch > 9){
		adcHw->SMPR1 &= ~(7 << ((ch - 10) * 3));
		adcHw->SMPR1 |= ADC_SampleHold << ((ch - 10) * 3);
	}else{
		adcHw->SMPR1 &= ~(7 << (ch * 3));
		adcHw->SMPR1 |= ADC_SampleHold << (ch * 3);
	}
}

static void tch_adcValidate(tch_adc_handle_prototype* ins){
	ins->status &= ~0xFFFF;
	ins->status |= (ADC_CLASS_KEY ^ ((uint32_t)ins & 0xFFFF));
}

static void tch_adcInvalidata(tch_adc_handle_prototype* ins){
	ins->status &= ~0xFFFF;
}

static BOOL tch_adcIsValid(tch_adc_handle_prototype* ins){
	return (ins->status & 0xFFFF) == (ADC_CLASS_KEY ^ ((uint32_t) ins & 0xFFFF));
}


static BOOL tch_adcHandleInterrupt(tch_adc_descriptor* adcDesc,tch_adc_handle_prototype* ins){
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(adcHw->SR & ins->isr_msk){
		if(adcHw->SR & ADC_SR_EOC){
			ins->env->MsgQ->put(ins->msgq,adcHw->DR,0);
			adcHw->SR &= ~ADC_SR_STRT;
			adcHw->CR2 &= ~ADC_CR2_EXTEN_0;
			return TRUE;
		}
		if(adcHw->SR & ADC_SR_OVR){
			ins->env->MsgQ->put(ins->msgq,ADC_READ_FAIL,0);
			return TRUE;
		}
	}
	return FALSE;
}

void ADC_IRQHandler(void){
	if(ADC_HWs[0]._handle){
		if(tch_adcHandleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
	if(ADC_HWs[1]._handle){
		if(tch_adcHandleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
	if(ADC_HWs[2]._handle){
		if(tch_adcHandleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
}
