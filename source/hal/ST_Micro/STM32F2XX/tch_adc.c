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


#include "hal/tch_hal.h"
#include "tch_halcfg.h"
#include "tch_halinit.h"
#include "tch_adc.h"


#define _1_MHZ                ((uint32_t) 1000000)
#define ADC_Precision_Pos     ((uint8_t) 24)
#define ADC_CR2_EXTSEL_Pos    ((uint8_t) 24)

#define ADC_CLASS_KEY         ((uint16_t) 0x4230)
#define ADC_FLAG_BUSY         ((uint32_t) 0x10000)

#define tch_ADC_Ch0_BIT               ((uint16_t) 0x1)
#define tch_ADC_Ch1_BIT               ((uint16_t) 0x2)
#define tch_ADC_Ch2_BIT               ((uint16_t) 0x4)
#define tch_ADC_Ch3_BIT               ((uint16_t) 0x8)
#define tch_ADC_Ch4_BIT               ((uint16_t) 0x10)
#define tch_ADC_Ch5_BIT               ((uint16_t) 0x20)
#define tch_ADC_Ch6_BIT               ((uint16_t) 0x40)
#define tch_ADC_Ch7_BIT               ((uint16_t) 0x80)
#define tch_ADC_Ch8_BIT               ((uint16_t) 0x100)
#define tch_ADC_Ch9_BIT               ((uint16_t) 0x200)
#define tch_ADC_Ch10_BIT              ((uint16_t) 0x400)
#define tch_ADC_Ch11_BIt              ((uint16_t) 0x800)


static tch_adcHandle* tch_adcOpen(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout);
static tchStatus tch_adcClose(tch_adcHandle* self);
static uint32_t tch_adcRead(const tch_adcHandle* self,uint8_t ch);
static tchStatus tch_adcStartBurstConvert(const tch_adcHandle* self,uint16_t ch,tch_mailqId q,uint32_t convCnt);
static tchStatus tch_adcPauseBurstConvert(const tch_adcHandle* self);
static tchStatus tch_adcStopBurstConvert(const tch_adcHandle* self);



static void tch_adcCfgInit(tch_adcCfg* cfg);
static void tch_adcAddChannel(tch_adcChDef* chdef,uint8_t ch);
static void tch_adcRemoveChannel(tch_adcChDef* chdef,uint8_t ch);
static BOOL tch_adcAvailable(adc_t adc);


/**
 * 	tch_adc_instance                _pix;
	uint16_t                         state;
	uint8_t                          idx;
	tch_dma_instance*               _dma_handle;
	tch_pwm_instance*               _tim_handle;
	tch_adc_evQue                    evqueue;
#ifdef FEATURE_MTHREAD
	tch_mtx_lock                         lock;
#endif
	tch_adc_iohandle                 io_handles[19];
 */



typedef struct tch_lld_adc_prototype {
	tch_lld_adc                          pix;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
}tch_lld_adc_prototype;


typedef struct tch_lld_adc_handle_prototype {
	tch_adcHandle                        pix;
	uint32_t                             status;
	uint32_t                             ch_occp;
	adc_t                                adc;
	tch_DmaHandle                        dma;
	tch_pwmHandle                        *timer;
	tch_mtxId                            mtx;
	tch_condvId                          condv;
	uint8_t                              chcnt;
	tch_GpioHandle                     **io_handle;
}tch_lld_adc_handle_prototype;



static void inline tch_adc_setRegChannel(tch_adc_descriptor* ins,uint8_t ch,uint8_t order)  __attribute__((always_inline));
static void inline tch_adc_setRegSampleHold(tch_adc_descriptor* ins,uint8_t ch,uint8_t ADC_SampleHold) __attribute__((always_inline));



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

const tch_lld_adc* tch_adc_instance = (tch_lld_adc*) &ADC_StaticInstance;

static tch_adcHandle* tch_adcOpen(const tch* env,adc_t adc,tch_adcCfg* cfg,tch_PwrOpt popt,uint32_t timeout){
	tch_lld_adc_handle_prototype* ins = NULL;
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
	tch_adc_com_bs* adc_common = &ADC_COM_BD_CFGs;
	tch_adc_bs* adcBs = &ADC_BD_CFGs[adc];
	ADC_TypeDef* adcHw = adcDesc->_hw;
	uint8_t smphld = 0;

	if(env->Mtx->lock(ADC_StaticInstance.mtx,timeout) != osOK)
		return NULL;
	while(adcDesc->_handle && (cfg->chdef.chselMsk& adc_common->occp_status)){
		if(env->Condv->wait(ADC_StaticInstance.condv,ADC_StaticInstance.mtx,timeout) != osOK)
			return NULL;
	}
	ins = adcDesc->_handle = env->Mem->alloc(sizeof(tch_lld_adc_handle_prototype));
	env->uStdLib->string->memset(ins,0,sizeof(tch_lld_adc_handle_prototype));
	adc_common->occp_status |= (ins->ch_occp = cfg->chdef.chselMsk);
	if(env->Mtx->unlock(ADC_StaticInstance.mtx) != osOK)
		return NULL;
	ins->io_handle = (tch_GpioHandle**) env->Mem->alloc(sizeof(tch_GpioHandle*) * cfg->chdef.chcnt);
	ins->mtx = env->Mtx->create();
	ins->condv = env->Condv->create();
	// gpio initialize
	tch_GpioCfg iocfg;
	env->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_AN;
	iocfg.PuPd = GPIO_PuPd_Float;


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
			if(adc_common->ports[ch_idx].adc_msk & (1 << adc)){
				ins->io_handle[ins->chcnt++] = env->Device->gpio->allocIo(env,adc_common->ports[ch_idx].port.port,(1 << adc_common->ports[ch_idx].port.pin),cfg,timeout,popt);
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
		ins->dma = env->Device->dma->allocDma(env,adcBs->dma,&dmacfg,timeout,popt);
	}else{
		// handle dma_not_used
	}

	tch_pwmDef pwmDef;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = _1_MHZ / cfg->SampleFreq;
	ins->timer = env->Device->timer->openPWM(env,adcBs->timer,&pwmDef,timeout);

	adcHw->CR1 |= (cfg->Precision << ADC_Precision_Pos) | ADC_CR1_OVRIE;
	adcHw->CR2 |= (ADC_CR2_ADON | (adcBs->timerExtsel << ADC_CR2_EXTSEL_Pos));

	ins->pix.close = tch_adcClose;
	ins->pix.pauseBurstConvert = tch_adcPauseBurstConvert;
	ins->pix.startBurstConvert = tch_adcStartBurstConvert;
	ins->pix.stopBurstConvert = tch_adcStopBurstConvert;
	ins->pix.read = tch_adcRead;

	return (tch_adcHandle*) ins;
}

static BOOL tch_adcAvailable(adc_t adc){
	return ADC_HWs[adc]._handle == NULL;
}


static tchStatus tch_adcClose(tch_adcHandle* self){

}

static uint32_t tch_adcRead(const tch_adcHandle* self,uint16_t ch){

}

static tchStatus tch_adcStartBurstConvert(const tch_adcHandle* self,uint16_t ch,tch_mailqId q,uint32_t convCnt){

}

static tchStatus tch_adcPauseBurstConvert(const tch_adcHandle* self){

}

static tchStatus tch_adcStopBurstConvert(const tch_adcHandle* self){

}


static void tch_adcCfgInit(tch_adcCfg* cfg){
	cfg->Precision = ADC_Precision_8B;
	cfg->SampleFreq = 1000;
	cfg->SampleHold = ADC_SampleHold_Mid;
	tch_adcAddChannel(&cfg->chdef,1);
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

static void tch_adcSetChannel(tch_adcChDef* chdef,uint16_t setchmsk){

}

static void tch_adcClearChannel(tch_adcChDef* chdef,uint16_t clrchmsk){

}

