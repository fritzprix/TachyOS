/*
 * tch_rtc.c
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



#include "tch_rtc.h"
#include "tch_hal.h"
#include "tch_halinit.h"
#include "tch_kernel.h"

#define RTC_CLASS_KEY               ((uint16_t) 0xAB5D)
#define RTC_LPMODE_Msk              ((uint32_t) 0x30000)

#define RTC_ACCESS_KEY1             ((uint8_t) 0xCA)
#define RTC_ACCESS_KEY2             ((uint8_t) 0x53)
#define HOUR_IN_SEC                 ((uint16_t) 3600)

#define RTC_CLK_LSE                 ((uint8_t) 1)
#define RTC_CLK_LSI                 ((uint8_t) 2)
#define RTC_CLK_HSE                 ((uint8_t) 3)
#define RTC_CLK_Msk                 ((uint8_t) 3)
#define RTC_CLK_Pos                 ((uint8_t) 8)

#define RTC_PRERA_Pos               ((uint8_t) 16)
#define RTC_PRERS_Pos               ((uint8_t) 0)

#define tch_rtcValidate(ins)        do{\
	((tch_rtc_handle_prototype*) ins)->status |= (((uint32_t) ins ^ RTC_CLASS_KEY) & 0xFFFF);\
}while(0)

#define tch_rtcInvalidate(ins)      do{\
	((tch_rtc_handle_prototype*) ins)->status &= ~0xFFFF;\
}while(0)

#define tch_rtcIsValid(ins)         (((tch_rtc_handle_prototype*) ins)->status & 0xFFFF) == (((uint32_t) ins ^ RTC_CLASS_KEY) & 0xFFFF)

typedef struct tch_lld_rtc_prototype {
	tch_lld_rtc                           pix;
	void*                                 _handle;
	tch_mtxId                             mtx;
	tch_condvId                           condv;
} tch_lld_rtc_prototype;

typedef struct tch_rtc_handle_prototype_t {
	tch_rtcHandle                         pix;
	uint32_t                              status;
	tch_mtxId                             mtx;
	const tch*                            env;
	tch_rtc_wkupHandler                   wkup_handler;
	time_t                                gmt_epoch;
	tch_timezone                          local_tz;
	uint32_t*                             bkp_regs;
	uint16_t                              wkup_period;
}tch_rtc_handle_prototype;


static tch_rtcHandle* tch_rtcOpen(const tch* env,time_t lt,tch_timezone tz);

static tchStatus tch_rtcClose(tch_rtcHandle* self);
static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t gmt_tm,tch_timezone tz,BOOL force);
static tchStatus tch_rtcGetTime(tch_rtcHandle* self,struct tm* ltm);
static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm,tch_alrRes resolution);
static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm);
static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler handler);
static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self);



__attribute__((section(".data"))) static tch_lld_rtc_prototype RTC_StaticInstance = {
		{
				tch_rtcOpen
		},
		NULL,
		NULL,
		NULL
};


const tch_lld_rtc* tch_rtc = (const tch_lld_rtc*) &RTC_StaticInstance;



static tch_rtcHandle* tch_rtcOpen(const tch* env,time_t gmt_epoch,tch_timezone tz){
	tch_rtc_handle_prototype* ins = NULL;

#if DBG
	DBGMCU->CR |= (DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP);
#endif


	if(!RTC_StaticInstance.condv)
		RTC_StaticInstance.condv = env->Condv->create();
	if(!RTC_StaticInstance.mtx)
		RTC_StaticInstance.mtx = env->Mtx->create();

	if(env->Mtx->lock(RTC_StaticInstance.mtx,osWaitForever) != tchOK)
		return NULL;

	while(RTC_StaticInstance._handle){
		if(env->Condv->wait(RTC_StaticInstance.condv,RTC_StaticInstance.mtx,osWaitForever) != tchOK)
			return NULL;
	}

	RTC_StaticInstance._handle = ins = (tch_rtc_handle_prototype*) env->Mem->alloc(sizeof(tch_rtc_handle_prototype));
	if(env->Mtx->unlock(RTC_StaticInstance.mtx) != tchOK)
		return NULL;
	env->uStdLib->string->memset(ins,0,sizeof(tch_rtc_handle_prototype));

	// RTC Clock source enable
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= PWR_CR_DBP;   //enable rtc register access


	while(!(RCC->BDCR & RCC_BDCR_LSERDY)) RCC->BDCR |= RCC_BDCR_LSEON;

	// RTC Clock enable
	RCC->BDCR |= ((RTC_CLK_LSE << RTC_CLK_Pos) | RCC_BDCR_RTCEN);

	RTC->WPR = RTC_ACCESS_KEY1;
	RTC->WPR = RTC_ACCESS_KEY2;    // input access-key

	if(RTC->ISR & RTC_ISR_INITS){

		RTC->ISR |= RTC_ISR_INIT;
		while(!(RTC->ISR & RTC_ISR_INITF)) __NOP();

		RTC->PRER = (255 << RTC_PRERS_Pos);  // setup rtc prescaler to obtain 1 Hz
		RTC->PRER |= (127 << RTC_PRERA_Pos);  // setup rtc prescaler to obtain 1 Hz

	}

	ins->mtx = env->Mtx->create();
	ins->pix.cancelAlarm = tch_rtcCancelAlarm;
	ins->pix.close = tch_rtcClose;
	ins->pix.disablePeriodicWakeup = tch_rtcDisablePeriodicWakeup;
	ins->pix.enablePeriodicWakeup = tch_rtcEnablePeriodicWakeup;
	ins->pix.getTime = tch_rtcGetTime;
	ins->pix.setTime = tch_rtcSetTime;
	ins->pix.setAlarm = tch_rtcSetAlarm;

	ins->env = env;
	tch_rtcValidate(ins);

	tch_rtcSetTime((tch_rtcHandle*) ins,gmt_epoch,tz,TRUE);

	RTC->ISR &= ~RTC_ISR_INIT;


	return (tch_rtcHandle*) ins;
}

static tchStatus tch_rtcClose(tch_rtcHandle* self){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	tch_rtcDisablePeriodicWakeup(self);

	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != tchOK)
		return tchErrorResource;
	tch_rtcInvalidate(ins);
	ins->env->Mtx->destroy(ins->mtx);

	if(ins->env->Mtx->lock(RTC_StaticInstance.mtx,osWaitForever) != tchOK)
		return tchErrorResource;

	NVIC_DisableIRQ(RTC_WKUP_IRQn);
	NVIC_DisableIRQ(RTC_Alarm_IRQn);

	RCC->BDCR |= RCC_BDCR_BDRST;
	RCC->BDCR &= RCC_BDCR_BDRST;
	RCC->BDCR &= ~RCC_BDCR_RTCEN;

	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;
	RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;
	RTC_StaticInstance._handle = 0;
	ins->env->Condv->wakeAll(RTC_StaticInstance.condv);
	ins->env->Mtx->unlock(RTC_StaticInstance.mtx);
	ins->env->Mem->free(ins);
	return tchOK;
}

static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t gmt_tm,tch_timezone tz,BOOL force){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	struct tm localTm;
	uint32_t dr = 0;
	uint32_t tr = 0;
	uint32_t tmp = 0;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;
	if((RTC->ISR & RTC_ISR_INITS) || !force)   // RTC is already initialized or not forced, it'll not be updated
		return tchOK;
	gmt_tm += tz * (HOUR_IN_SEC);
	localtime_r(&gmt_tm,&localTm);
	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != tchOK)
		return tchErrorResource;
	RTC->ISR |= RTC_ISR_INIT;
	while(!(RTC->ISR & RTC_ISR_INITF))
		/*NOP*/;
	RTC->TR = 0;
	RTC->DR = 0;
	ins->local_tz = tz;
	ins->gmt_epoch = gmt_tm;
	tmp =  localTm.tm_hour / 10;
	tr |= ((tmp << 20) | ((localTm.tm_hour - (tmp * 10)) << 16));
	tmp = localTm.tm_min / 10;
	tr |= ((tmp << 12) | ((localTm.tm_min - (tmp * 10)) << 8));
	tmp = localTm.tm_sec / 10;
	tr |= ((tmp << 4) | (localTm.tm_sec - (tmp * 10)));
	RTC->TR = tr;

	tmp = localTm.tm_year / 10;
	dr |= ((tmp << 20) | ((localTm.tm_year - (tmp * 10)) << 16));
	tmp = localTm.tm_wday == 0? 7 : localTm.tm_wday;
	dr |= (tmp << 13);
	tmp = (localTm.tm_mon + 1) / 10;
	dr |= ((tmp << 12) | (((localTm.tm_mon + 1) - (tmp * 10)) << 8));
	tmp = localTm.tm_mday / 10;
	dr |= ((tmp << 4) | (localTm.tm_mday - (tmp * 10)));
	RTC->DR = dr;


	RTC->PRER = (255 << RTC_PRERS_Pos);  // setup rtc prescaler to obtain 1 Hz
	RTC->PRER |= (127 << RTC_PRERA_Pos);  // setup rtc prescaler to obtain 1 Hz


	RTC->ISR &= ~RTC_ISR_INIT;

	if(ins->env->Mtx->unlock(ins->mtx) != tchOK)
		return tchErrorResource;
	return tchOK;
}

static tchStatus tch_rtcGetTime(tch_rtcHandle* self,struct tm* ltm){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	tchStatus result;
	uint32_t date, time = 0;
	time_t lt;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	if((result = ins->env->Mtx->lock(ins->mtx,osWaitForever)) != tchOK)
		return result;

	date = RTC->DR;
	time = RTC->TR;

	if(time & RTC_TR_PM)
		RTC->TR &= ~RTC_TR_PM;
	ins->env->uStdLib->string->memset(ltm,0,sizeof(struct tm));

	ltm->tm_hour += ((time & RTC_TR_HT) >> 20) * 10;
	ltm->tm_hour += ((time & RTC_TR_HU) >> 16);

	ltm->tm_min += ((time & RTC_TR_MNT) >> 12) * 10;
	ltm->tm_min += ((time & RTC_TR_MNU) >> 8);

	ltm->tm_sec += ((time & RTC_TR_ST) >> 4) * 10;
	ltm->tm_sec += (time & RTC_TR_SU);

	ltm->tm_year += (((date & RTC_DR_YT) >> 20) * 10);
	ltm->tm_year += ((date & RTC_DR_YU) >> 16);

	ltm->tm_mon += ((date & RTC_DR_MT) >> 12) * 10;
	ltm->tm_mon += ((date & RTC_DR_MU) >> 8);
	ltm->tm_mon--;

	ltm->tm_mday += ((date & RTC_DR_DT) >> 4) * 10;
	ltm->tm_mday += (date & RTC_DR_DU);

	lt = mktime(ltm);                                     // get updated ltm and epoch
	ins->gmt_epoch = lt - (HOUR_IN_SEC * ins->local_tz);   // update gmt epoch

	if(time & RTC_TR_PM)
		RTC->TR |= RTC_TR_PM;
	ins->env->Mtx->unlock(ins->mtx);
	return tchOK;
}

static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm,tch_alrRes resolution){

}

static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm){

}

static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler wkup_handler){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;
	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != tchOK)
		return tchErrorResource;

	ins->wkup_period = periodInSec;

	EXTI->IMR |= (1 << 22);
	EXTI->RTSR |= (1 << 22);
	RTC->CR &= ~(7 | RTC_CR_WUTE); // clear wuclk
	while(!(RTC->ISR & RTC_ISR_WUTWF))
		/*NOP*/;
	RTC->WUTR = periodInSec - 1;
	RTC->CR |= 4;  // set rtc / 16 : 2048 Hz
	RTC->CR |= RTC_CR_WUTIE;
	RTC->CR |= RTC_CR_WUTE;
	RTC->ISR &= ~RTC_ISR_WUTF;
	ins->wkup_handler = wkup_handler;

	NVIC_SetPriority(RTC_WKUP_IRQn,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(RTC_WKUP_IRQn);

	return ins->env->Mtx->unlock(ins->mtx);

}

static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != tchOK)
		return tchErrorResource;

	RTC->CR &= ~7; // clear wuclk
	RTC->CR |= 0;  // set rtc / 16 : 2048 Hz
	RTC->CR &= ~RTC_CR_WUTE;
	RTC->CR &= ~RTC_CR_WUTIE;
	EXTI->IMR &= ~(1 << 22);
	EXTI->RTSR &= ~(1 << 22);
	ins->wkup_handler = NULL;

	NVIC_DisableIRQ(RTC_WKUP_IRQn);

	return ins->env->Mtx->unlock(ins->mtx);

}


void RTC_Alarm_IRQHandler(){

}

void RTC_WKUP_IRQHandler(){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)RTC_StaticInstance._handle;
	if(!ins)
		return;
	if(!tch_rtcIsValid(ins))
		return;
	ins->gmt_epoch += ins->wkup_period;
	*((time_t*)ins->bkp_regs) = ins->gmt_epoch;
	if(ins->wkup_handler)
		ins->wkup_handler();
	EXTI->PR |= (1 << 22);
	RTC->ISR &= ~RTC_ISR_WUTF;
	PWR->CR |= PWR_CR_CWUF;

}
