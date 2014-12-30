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
	void*                                 occp;
	tch_mtxId                             mtx;
	tch_condvId                           condv;
} tch_lld_rtc_prototype;

typedef struct tch_rtc_handle_prototype_t {
	tch_rtcHandle                         pix;
	uint32_t                              status;
	tch_mtxId                             mtx;
	tch_barId                             wkup_barrier;
	const tch*                            env;
}tch_rtc_handle_prototype;

struct tch_tm {
	  uint8_t	tm_sec;
	  uint8_t	tm_min;
	  uint8_t	tm_hour;
	  uint8_t	tm_mday;
	  uint8_t	tm_mon;
	  uint32_t	tm_year;
	  uint8_t	tm_wday;
	  uint16_t	tm_yday;
}__attribute__((packed));




static tch_rtcHandle* tch_rtcOpen(const tch* env,time_t lt,tch_timezone tz);

static tchStatus tch_rtcClose(tch_rtcHandle* self);
static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t gmt_tm,tch_timezone tz,BOOL force);
static time_t tch_rtcGetTime(tch_rtcHandle* self,struct tm* ltm);
static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm);
static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm);
static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec);
static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self);
static tchStatus tch_rtcWaitUntilWakeup(tch_rtcHandle* self);



__attribute__((section(".data"))) static tch_lld_rtc_prototype RTC_StaticInstance = {
		{
				tch_rtcOpen
		},
		0,
		NULL,
		NULL
};


const tch_lld_rtc* tch_rtc = (const tch_lld_rtc*) &RTC_StaticInstance;



static tch_rtcHandle* tch_rtcOpen(const tch* env,time_t gmt_epoch,tch_timezone tz){
	tch_rtc_handle_prototype* ins = NULL;


	if(!RTC_StaticInstance.condv)
		RTC_StaticInstance.condv = env->Condv->create();
	if(!RTC_StaticInstance.mtx)
		RTC_StaticInstance.mtx = env->Mtx->create();

	if(env->Mtx->lock(RTC_StaticInstance.mtx,osWaitForever) != osOK)
		return NULL;

	while(RTC_StaticInstance.occp){
		if(env->Condv->wait(RTC_StaticInstance.condv,RTC_StaticInstance.mtx,osWaitForever) != osOK)
			return NULL;
	}

	RTC_StaticInstance.occp = ins = (tch_rtc_handle_prototype*) env->Mem->alloc(sizeof(tch_rtc_handle_prototype));
	if(env->Mtx->unlock(RTC_StaticInstance.mtx) != osOK)
		return NULL;
	env->uStdLib->string->memset(ins,0,sizeof(tch_rtc_handle_prototype));

	// RTC Clock source enable
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= PWR_CR_DBP;   //enable rtc register access

	RCC->BDCR |= RCC_BDCR_BDRST;
	RCC->BDCR &= ~RCC_BDCR_BDRST;

	RCC->BDCR |= RCC_BDCR_LSEON;
	while(!(RCC->BDCR & RCC_BDCR_LSERDY))
		/*NOP*/ ;

	// RTC Clock enable
	RCC->BDCR |= ((RTC_CLK_LSE << RTC_CLK_Pos) | RCC_BDCR_RTCEN);

	RTC->WPR = RTC_ACCESS_KEY1;
	RTC->WPR = RTC_ACCESS_KEY2;    // input access-key

	RTC->ISR |= RTC_ISR_INIT;
	while(!(RTC->ISR & RTC_ISR_INITF))
		/*NOP*/;

	RTC->PRER = (255 << RTC_PRERS_Pos);  // setup rtc prescaler to obtain 1 Hz
	RTC->PRER |= (127 << RTC_PRERA_Pos);  // setup rtc prescaler to obtain 1 Hz


	ins->mtx = env->Mtx->create();
	ins->wkup_barrier = env->Barrier->create();
	ins->pix.cancelAlarm = tch_rtcCancelAlarm;
	ins->pix.close = tch_rtcClose;
	ins->pix.disablePeriodicWakeup = tch_rtcDisablePeriodicWakeup;
	ins->pix.enablePeriodicWakeup = tch_rtcEnablePeriodicWakeup;
	ins->pix.getTime = tch_rtcGetTime;
	ins->pix.setTime = tch_rtcSetTime;
	ins->pix.setAlarm = tch_rtcSetAlarm;
	ins->pix.waitUntilWakeup = tch_rtcWaitUntilWakeup;

	ins->env = env;
	tch_rtcValidate(ins);

	tch_rtcSetTime((tch_rtcHandle*) ins,gmt_epoch,tz,TRUE);

	RTC->ISR &= RTC_ISR_INIT;


	return (tch_rtcHandle*) ins;
}

static tchStatus tch_rtcClose(tch_rtcHandle* self){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return osErrorParameter;
	if(!tch_rtcIsValid(ins))
		return osErrorParameter;

	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != osOK)
		return osErrorResource;
	tch_rtcInvalidate(ins);
	ins->env->Mtx->destroy(ins->mtx);
	ins->env->Barrier->destroy(ins->wkup_barrier);

	if(ins->env->Mtx->lock(RTC_StaticInstance.mtx,osWaitForever) != osOK)
		return osErrorResource;
	NVIC_DisableIRQ(RTC_HW.irq_0);
	NVIC_DisableIRQ(RTC_HW.irq_1);

	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;
	RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;
	RTC_StaticInstance.occp = 0;
	ins->env->Condv->wakeAll(RTC_StaticInstance.condv);
	ins->env->Mtx->unlock(RTC_StaticInstance.mtx);
	ins->env->Mem->free(ins);
	return osOK;
}

static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t gmt_tm,tch_timezone tz,BOOL force){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	struct tm localTm;
	uint32_t dr = 0;
	uint32_t tr = 0;
	uint32_t tmp = 0;
	if(!ins)
		return osErrorParameter;
	if(!tch_rtcIsValid(ins))
		return osErrorParameter;
	if((RTC->ISR & RTC_ISR_INITS) && !force)   // RTC is already initialized, and not forced, it'll not be updated
		return osOK;
	gmt_tm += tz * (HOUR_IN_SEC);
	localtime_r(&gmt_tm,&localTm);
	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != osOK)
		return osErrorResource;
	RTC->ISR |= RTC_ISR_INIT;
	while(!(RTC->ISR & RTC_ISR_INITF))
		/*NOP*/;
	RTC->TR = 0;
	RTC->DR = 0;
	tmp =  localTm.tm_hour / 10;
	tr |= ((tmp << 20) | ((localTm.tm_hour - (tmp * 10)) << 16));
	tmp = localTm.tm_min / 10;
	tr |= ((tmp << 12) | ((localTm.tm_min - (tmp * 10)) << 8));
	tmp = localTm.tm_sec / 10;
	tr |= ((tmp << 4) | (localTm.tm_sec - (tmp * 10)));
	RTC->TR = tr;

	localTm.tm_year += 1900;
	tmp = localTm.tm_year / 10;
	dr |= ((tmp << 20) | ((localTm.tm_year - (tmp * 10)) << 16));
	tmp = localTm.tm_wday == 0? 7 : localTm.tm_wday;
	dr |= (tmp << 13);
	tmp = (localTm.tm_mon + 1) / 10;
	dr |= ((tmp << 12) | (((localTm.tm_mon + 1) - (tmp * 10)) << 8));
	tmp = localTm.tm_mday / 10;
	dr |= ((tmp << 4) | (localTm.tm_mday - (tmp * 10)));
	RTC->DR = dr;
	RTC->ISR &= ~RTC_ISR_INIT;

	if(ins->env->Mtx->unlock(ins->mtx) != osOK)
		return osErrorResource;
	return osOK;
}

static time_t tch_rtcGetTime(tch_rtcHandle* self,struct tm* ltm){

}

static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm){

}

static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm){

}

static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return osErrorParameter;
	if(!tch_rtcIsValid(ins))
		return osErrorParameter;
	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != osOK)
		return osErrorResource;

	RTC->CR &= ~(7 | RTC_CR_WUTE); // clear wuclk
	while(!(RTC->ISR & RTC_ISR_WUTWF))
		/*NOP*/;
	RTC->WUTR = periodInSec;
	RTC->CR |= 4;  // set rtc / 16 : 2048 Hz
	RTC->CR |= RTC_CR_WUTIE;
	RTC->CR |= RTC_CR_WUTE;
	RTC->ISR &= ~RTC_ISR_WUTF;



	NVIC_SetPriority(RTC_WKUP_IRQn,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(RTC_WKUP_IRQn);

	return ins->env->Mtx->unlock(ins->mtx);

}

static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return osErrorParameter;
	if(!tch_rtcIsValid(ins))
		return osErrorParameter;

	if(ins->env->Mtx->lock(ins->mtx,osWaitForever) != osOK)
		return osErrorResource;

	RTC->CR &= ~7; // clear wuclk
	RTC->CR |= 0;  // set rtc / 16 : 2048 Hz
	RTC->CR &= ~RTC_CR_WUTE;
	RTC->CR &= ~RTC_CR_WUTIE;

	NVIC_DisableIRQ(RTC_WKUP_IRQn);

	return ins->env->Mtx->unlock(ins->mtx);

}

static tchStatus tch_rtcWaitUntilWakeup(tch_rtcHandle* self){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	tchStatus result = osOK;
	if(!ins)
		return osErrorParameter;
	if(!tch_rtcIsValid(ins))
		return osErrorParameter;
	if((result = ins->env->Barrier->wait(ins->wkup_barrier,osWaitForever)) != osOK)
		return result;
	return osOK;
}


void RTC_Alarm_IRQHandler(){

}

void RTC_WKUP_IRQHandler(){
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)RTC_StaticInstance.occp;
	RTC->ISR &= ~RTC_ISR_WUTF;
	if(!ins)
		return;
	if(!tch_rtcIsValid(ins))
		return;
	ins->env->Barrier->signal(ins->wkup_barrier,osOK);
}
