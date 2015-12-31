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
#include "tch_gpio.h"
#include "tch_adc.h"
#include "tch_timer.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/util/string.h"

#define SET_SAFE_EXIT();      RETURN_EXIT:
#define RETURN_SAFE()         goto RETURN_EXIT
#define _1_MHZ                ((uint32_t) 1000000)
#define ADC_Precision_Pos     ((uint8_t) 24)
#define ADC_CR2_EXTSEL_Pos    ((uint8_t) 24)

#define ADC_CLASS_KEY         ((uint16_t) 0x4230)
#define ADC_BUSY_FLAG         ((uint32_t) 0x10000)


#define ADC_setBusy(ins)             do{\
	((tch_adc_handle_prototype*) ins)->status |= ADC_BUSY_FLAG;\
	set_system_busy();\
}while(0)

#define ADC_clrBusy(ins)             do{\
	((tch_adc_handle_prototype*) ins)->status &= ~ADC_BUSY_FLAG;\
	clear_system_busy();\
}while(0)

#define ADC_isBusy(ins)              ((tch_adc_handle_prototype*) ins)->status & ADC_BUSY_FLAG



typedef struct tch_adc_handle_prototype {
	tch_adcHandle                        pix;
	const tch*                           env;
	uint32_t                             status;
	uint16_t                             isr_msk;
	uint32_t                             ch_occp;
	adc_t                                adc;
	tch_dmaHandle                        dma;
	tch_pwmHandle*                       timer;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
	uint8_t                              chcnt;
	tch_gpioHandle**                     io_handle;
	tch_msgqId                           msgq;
} tch_adc_handle_prototype;


__USER_API__ static tch_adcHandle* tch_adc_open(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout);
__USER_API__ static tchStatus tch_adc_close(tch_adcHandle* self);
__USER_API__ static uint32_t tch_adc_read(const tch_adcHandle* self,uint8_t ch);
__USER_API__ static tchStatus tch_adc_burstConvert(const tch_adcHandle* self,uint8_t ch,tch_mailqId q,uint32_t qchnk,uint32_t convCnt);
static uint32_t tch_adcChannelOccpStatus;


__USER_API__ static void tch_adc_cfgInit(tch_adcCfg* cfg);
__USER_API__ static void tch_adc_addChannel(tch_adcCfg* cfg,uint8_t ch);
__USER_API__ static void tch_adc_removeChannel(tch_adcCfg* cfg,uint8_t ch);
__USER_API__ static BOOL tch_adc_isAvailable(adc_t adc);

static int tch_adc_init(void);
static void tch_adc_exit(void);
static void tch_adc_validate(tch_adc_handle_prototype* ins);
static void tch_adc_invalidata(tch_adc_handle_prototype* ins);
static BOOL tch_adc_isValid(tch_adc_handle_prototype* ins);
static BOOL tch_adc_handleInterrupt(tch_adc_descriptor* adcDesc,tch_adc_handle_prototype* ins);

static void tch_adc_setRegChannel(tch_adc_descriptor* ins,uint8_t ch,uint8_t order);
static void tch_adc_setRegSampleHold(tch_adc_descriptor* ins,uint8_t ch,uint8_t ADC_SampleHold);


__USER_RODATA__ tch_device_service_adc ADC_Ops = {
		.count = MFEATURE_ADC,
		.max_precision = 12,
		.min_precision = 6,
		.initCfg = tch_adc_cfgInit,
		.addChannel = tch_adc_addChannel,
		.removeChannel = tch_adc_removeChannel,
		.open = tch_adc_open,
		.available = tch_adc_isAvailable
};

static tch_mtxCb ADC_Mutex;
static tch_condvCb ADC_Condv;
static tch_device_service_dma* dma;

static int tch_adc_init(void)
{
	if(!MFEATURE_ADC)
		return FALSE;

	dma = NULL;
	tch_adcChannelOccpStatus = 0;
	tch_mutexInit(&ADC_Mutex);
	tch_condvInit(&ADC_Condv);
	tch_kmod_register(MODULE_TYPE_ANALOGIN,ADC_CLASS_KEY,&ADC_Ops,FALSE);

	return  TRUE;
}

static void tch_adc_exit(void)
{
	tch_mutexDeinit(&ADC_Mutex);
	tch_condvInit(&ADC_Condv);
	tch_kmod_unregister(MODULE_TYPE_ANALOGIN,ADC_CLASS_KEY);
}


MODULE_INIT(tch_adc_init);
MODULE_EXIT(tch_adc_exit);

static tch_adcHandle* tch_adc_open(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout)
{
	tch_adc_handle_prototype* ins = NULL;
	uint8_t ch_idx = 0;
	tch_DmaCfg dmacfg;
	if(adc > MFEATURE_ADC)
		return NULL;
	if(!(cfg->chdef.chselMsk > 0))
		return NULL;

	tch_device_service_timer* timer = (tch_device_service_timer*) Service->request(MODULE_TYPE_TIMER);
	tch_device_service_gpio* gpio = (tch_device_service_gpio*) Service->request(MODULE_TYPE_GPIO);

	if(!dma)
		dma = (tch_device_service_dma*) tch_kmod_request(MODULE_TYPE_DMA);

	if(!timer || !gpio)
		return NULL;

	tch_adc_descriptor* adcDesc = &ADC_HWs[adc];
	tch_adc_bs_t* adcBs = &ADC_BD_CFGs[adc];
	tch_adc_channel_bs_t* adcChBs = NULL;
	ADC_TypeDef* adcHw = adcDesc->_hw;
	uint8_t smphld = 0;

	if(env->Mtx->lock(&ADC_Mutex,timeout) != tchOK)
		return NULL;
	while(adcDesc->_handle && (cfg->chdef.chselMsk& tch_adcChannelOccpStatus))
	{
		if(env->Condv->wait(&ADC_Condv,&ADC_Mutex,timeout) != tchOK)
			return NULL;
	}

	ins = adcDesc->_handle = env->Mem->alloc(sizeof(tch_adc_handle_prototype));
	mset(ins,0,sizeof(tch_adc_handle_prototype));
	tch_adcChannelOccpStatus |= (ins->ch_occp = cfg->chdef.chselMsk);
	if(env->Mtx->unlock(&ADC_Mutex) != tchOK)
		return NULL;

	ins->io_handle = (tch_gpioHandle**) env->Mem->alloc(sizeof(tch_gpioHandle*) * cfg->chdef.chcnt);
	ins->mtx = env->Mtx->create();
	ins->condv = env->Condv->create();
	ins->msgq = env->MsgQ->create(1);
	// gpio initialize

	gpio_config_t iocfg;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_AN;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.popt = popt;


	*adcDesc->_clkenr |= adcDesc->clkmsk;

	if(popt == ActOnSleep)
	{
		*adcDesc->_lpclkenr |= adcDesc->lpclkmsk;
	}

	switch(cfg->SampleHold)
	{
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

	for(ch_idx = 0,ins->chcnt = 0;(cfg->chdef.chselMsk && (cfg->chdef.chcnt)); )
	{
		if(cfg->chdef.chselMsk & 1)
		{
			cfg->chdef.chcnt--;
			adcChBs = &ADC_CH_BD_CFGs[ch_idx];
			if(adcChBs->adc_routemsk & (1 << adc))
			{
				ins->io_handle[ins->chcnt++] = gpio->allocIo(env,adcChBs->port.port,(1 << adcChBs->port.pin),&iocfg,timeout);
				tch_adc_setRegSampleHold(adcDesc,ch_idx,smphld);
				tch_adc_setRegChannel(adcDesc,ch_idx,0);
			}
		}
		cfg->chdef.chselMsk >>= 1;
		ch_idx++;
	}


	if((adcBs->dma != DMA_NOT_USED) && dma)
	{
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
		ins->dma = dma->allocate(env,adcBs->dma,&dmacfg,timeout,popt);
	}
	else
	{
		// TODO :handle dma_not_used
	}

	pwm_config_t pwmDef;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = _1_MHZ / cfg->SampleFreq;
	pwmDef.pwrOpt = popt;


	ins->timer = timer->openPWM(env,adcBs->timer,&pwmDef,timeout);

	ins->isr_msk |= ADC_SR_OVR | ADC_SR_EOC;
	adcHw->CR1 |= (cfg->Precision << ADC_Precision_Pos) | ADC_CR1_OVRIE | ADC_CR1_EOCIE;
	adcHw->CR2 |= (ADC_CR2_ADON | (adcBs->timerExtsel << ADC_CR2_EXTSEL_Pos));

	ins->pix.close = tch_adc_close;
	ins->pix.readBurst = tch_adc_burstConvert;
	ins->pix.read = tch_adc_read;
	ins->env = env;


	tch_enableInterrupt(adcDesc->irq,HANDLER_NORMAL_PRIOR);
	tch_adc_validate(ins);
	return (tch_adcHandle*) ins;
}


static BOOL tch_adc_isAvailable(adc_t adc)
{
	if(adc >= MFEATURE_ADC)
		return FALSE;
	return ADC_HWs[adc]._handle == NULL;
}


static tchStatus tch_adc_close(tch_adcHandle* self)
{
	tch_adc_handle_prototype* ins =(tch_adc_handle_prototype*) self;
	tch_adc_descriptor* adcDesc = &ADC_HWs[ins->adc];
	tchStatus  result = tchOK;
	if(!self)
		return tchErrorParameter;
	if(!tch_adc_isValid(ins))
		return tchErrorParameter;
	const tch* env = ins->env;
	if((result = env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
		return result;

	while(ADC_isBusy(ins))
	{
		if((result = env->Condv->wait(ins->condv,ins->mtx,tchWaitForever)) != tchOK)
			return result;
	}
	tch_adc_invalidata(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);

	while(ins->chcnt--)
	{
		ins->io_handle[ins->chcnt]->close(ins->io_handle[ins->chcnt]);
	}
	env->Mem->free(ins->io_handle);
	ins->timer->close(ins->timer);

	if(dma)
		dma->freeDma(ins->dma);

	if((result = env->Mtx->lock(&ADC_Mutex,tchWaitForever)) != tchOK)
		return result;

	adcDesc->_handle = NULL;
	tch_adcChannelOccpStatus &= ins->ch_occp;
	tch_disableInterrupt(adcDesc->irq);
	env->Condv->wakeAll(&ADC_Condv);
	env->Mtx->unlock(&ADC_Mutex);
	env->Mem->free(ins);
	return tchOK;
}

static uint32_t tch_adc_read(const tch_adcHandle* self,uint8_t ch)
{
	tch_adc_handle_prototype* ins = (tch_adc_handle_prototype*) self;
	tch_adc_descriptor* adcDesc = &ADC_HWs[ins->adc];
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	tchEvent evt;
	if(!ins)
		return ADC_READ_FAIL;
	if(!tch_adc_isValid(ins))
		return ADC_READ_FAIL;
	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return ADC_READ_FAIL;

	while(ADC_isBusy(ins))
	{
		if(ins->env->Condv->wait(ins->condv,ins->mtx,tchWaitForever) != tchOK)
			return ADC_READ_FAIL;
	}
	ADC_setBusy(ins);
	if(ins->env->Mtx->unlock(ins->mtx) != tchOK)
		return ADC_READ_FAIL;

	tch_adc_setRegChannel(adcDesc,ch,1);
	adcHw->CR2 |= ADC_CR2_SWSTART;    /// start conversion
	evt = ins->env->MsgQ->get(ins->msgq,tchWaitForever);

	if(evt.status != tchEventMessage)
	{
		evt.value.v = ADC_READ_FAIL;
		RETURN_SAFE();
	}
	SET_SAFE_EXIT();
	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	ADC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return evt.value.v;
}

static tchStatus tch_adc_burstConvert(const tch_adcHandle* self,uint8_t ch, tch_mailqId q, uint32_t chnksz, uint32_t convCnt){
	tch_adc_handle_prototype* ins = (tch_adc_handle_prototype*) self;
	ADC_TypeDef* adcHw = NULL;
	tch_adc_bs_t* adcBs = NULL;
	tchEvent evt;
	tchStatus result = tchOK;
	tch_adc_descriptor* adcDesc = NULL;
	if(!ins)
		return tchErrorParameter;
	if(!tch_adc_isValid(ins))
		return tchErrorParameter;

	adcBs = &ADC_BD_CFGs[ins->adc];
	adcDesc = &ADC_HWs[ins->adc];
	adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if((result = ins->env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
		return result;
	while(ADC_isBusy(ins))
	{
		if((result = ins->env->Condv->wait(ins->condv,ins->mtx,tchWaitForever)) != tchOK)
			return result;
	}
	ADC_setBusy(ins);

	if((result = ins->env->Mtx->unlock(ins->mtx)) != tchOK)
		return result;

	ins->timer->setDuty(ins->timer,adcBs->timerCh,0.5f);

	tch_adc_setRegChannel(adcDesc,ch,1);
	//enable dma
	adcHw->CR2 |= ADC_CR2_DMA;
	adcHw->CR2 |= ADC_CR2_EXTEN_0;
	tch_DmaReqDef dmaReq;
	uint16_t* chnk = NULL;
	while(convCnt--)
	{
		chnk = (uint16_t*) ins->env->MailQ->alloc(q,tchWaitForever,NULL);
		dma->initReq(&dmaReq,chnk,(uint32_t*) &adcHw->DR,(chnksz / 2));    // burst unit is half word
		ins->timer->start(ins->timer);
		dma->beginXfer(ins->dma,&dmaReq,0,&result);
		evt = ins->env->MsgQ->get(ins->msgq,tchWaitForever);
		ins->timer->stop(ins->timer);
		ins->env->MailQ->put(q,chnk,1000);
		if(evt.status != tchEventMessage){
			evt.status = tchErrorIo;
			RETURN_SAFE();
		}
	}
	evt.status = tchOK;
	SET_SAFE_EXIT();
	adcHw->CR2 &= ~ADC_CR2_DMA;
	adcHw->CR2 &= ~ADC_CR2_EXTEN_0;
	ins->env->Mtx->lock(ins->mtx,tchWaitForever);
	ADC_clrBusy(ins);
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtx);
	return evt.status;
}



static void tch_adc_cfgInit(tch_adcCfg* cfg)
{
	cfg->chdef.chcnt = cfg->chdef.chselMsk = 0;
	cfg->Precision = ADC_Precision_8B;
	cfg->SampleFreq = 1000;
	cfg->SampleHold = ADC_SampleHold_Mid;
}

static void tch_adc_addChannel(tch_adcCfg* cfg,uint8_t ch)
{
	cfg->chdef.chselMsk |= (1 << ch);
	cfg->chdef.chselMsk |= (1 << ch);
	cfg->chdef.chcnt++;
}

static void tch_adc_removeChannel(tch_adcCfg* cfg,uint8_t ch)
{
	cfg->chdef.chselMsk &= ~(1 << ch);
	cfg->chdef.chcnt--;
}


static void tch_adc_setRegChannel(tch_adc_descriptor* adcDesc,uint8_t ch,uint8_t order)
{
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(order > 12){
		adcHw->SQR1 |= (ch << ((order - 13) * 4));
	}else if(order > 6){
		adcHw->SQR2 |= (ch << ((order - 7) * 4));
	}else{
		adcHw->SQR3 |= (ch << (order * 4));
	}
}


static void tch_adc_setRegSampleHold(tch_adc_descriptor* adcDesc,uint8_t ch,uint8_t ADC_SampleHold)
{
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(ch > 9){
		adcHw->SMPR1 &= ~(7 << ((ch - 10) * 3));
		adcHw->SMPR1 |= ADC_SampleHold << ((ch - 10) * 3);
	}else{
		adcHw->SMPR1 &= ~(7 << (ch * 3));
		adcHw->SMPR1 |= ADC_SampleHold << (ch * 3);
	}
}

static void tch_adc_validate(tch_adc_handle_prototype* ins)
{
	ins->status &= ~0xFFFF;
	ins->status |= (ADC_CLASS_KEY ^ ((uint32_t)ins & 0xFFFF));
}

static void tch_adc_invalidata(tch_adc_handle_prototype* ins)
{
	ins->status &= ~0xFFFF;
}

static BOOL tch_adc_isValid(tch_adc_handle_prototype* ins)
{
	return (ins->status & 0xFFFF) == (ADC_CLASS_KEY ^ ((uint32_t) ins & 0xFFFF));
}


static BOOL tch_adc_handleInterrupt(tch_adc_descriptor* adcDesc,tch_adc_handle_prototype* ins)
{
	ADC_TypeDef* adcHw = (ADC_TypeDef*) adcDesc->_hw;
	if(adcHw->SR & ins->isr_msk)
	{
		if(adcHw->SR & ADC_SR_EOC)
		{
			ins->env->MsgQ->put(ins->msgq,adcHw->DR,0);
			adcHw->SR &= ~ADC_SR_STRT;
			adcHw->CR2 &= ~ADC_CR2_EXTEN_0;
			return TRUE;
		}
		if(adcHw->SR & ADC_SR_OVR)
		{
			ins->env->MsgQ->put(ins->msgq,ADC_READ_FAIL,0);
			return TRUE;
		}
	}
	return FALSE;
}

void ADC_IRQHandler(void)
{
	if(ADC_HWs[0]._handle)
	{
		if(tch_adc_handleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
	if(ADC_HWs[1]._handle)
	{
		if(tch_adc_handleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
	if(ADC_HWs[2]._handle)
	{
		if(tch_adc_handleInterrupt(&ADC_HWs[0],ADC_HWs[0]._handle))
			return;
	}
}
