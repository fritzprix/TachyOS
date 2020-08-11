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
#include "kernel/tch_interrupt.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/string.h"

#ifndef RTC_CLASS_KEY
#define RTC_CLASS_KEY               ((uint16_t) 0xAB5D)
#endif

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


typedef struct tch_rtc_handle_prototype_t {
	tch_rtcHandle 						pix;
	uint32_t  							status;
	tch_mtxId 							mtx;
	const tch_core_api_t* 							env;
	tch_rtc_wkupHandler					wkup_handler;
	uint16_t							wkup_period;
	dlistEntry_t                        alrm_queue;
}tch_rtc_handle_prototype;

struct rtc_alarm_event
{
	dlistNode_t                         alrm_wnode;
	tch_thread_queue 					wait_queue;
	time_t								alrm_epoch_time;
};

static int tch_rtc_init(void);
static void tch_rtc_exit(void);



static tch_rtcHandle* tch_rtc_open(const tch_core_api_t* env,struct tm* localtm);
static tchStatus tch_rtc_close(tch_rtcHandle* self);
static tchStatus tch_rtc_setTime(tch_rtcHandle* self,struct tm* localtm,BOOL force);
static tchStatus tch_rtc_getTime(tch_rtcHandle* self,struct tm* localtm);
static tchStatus tch_rtc_enablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler handler);
static tchStatus tch_rtc_disablePeriodicWakeup(tch_rtcHandle* self);



__USER_RODATA__ tch_hal_module_rtc_t RTC_Ops = {
		.open = tch_rtc_open
};

static void* _handle;
static tch_mtxCb mtx;
static tch_condvCb condv;


static int tch_rtc_init(void)
{

	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;


	tch_mutexInit(&mtx);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_RTC,RTC_CLASS_KEY,&RTC_Ops,TRUE);
}

static void tch_rtc_exit(void)
{
	RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;

	tch_mutexDeinit(&mtx);
	tch_condvDeinit(&condv);
	tch_kmod_unregister(MODULE_TYPE_RTC,RTC_CLASS_KEY);
}

MODULE_INIT(tch_rtc_init);
MODULE_EXIT(tch_rtc_exit);



static tch_rtcHandle* tch_rtc_open(const tch_core_api_t* env,struct tm* ltm)
{
	tch_rtc_handle_prototype* ins = NULL;
	if(env->Mtx->lock(&mtx,tchWaitForever) != tchOK)
		return NULL;

	while(_handle)
	{
		if(env->Condv->wait(&condv,&mtx,tchWaitForever) != tchOK)
			return NULL;
	}


	_handle = ins = (tch_rtc_handle_prototype*) env->Mem->alloc(sizeof(tch_rtc_handle_prototype));
	if(env->Mtx->unlock(&mtx) != tchOK)
		return NULL;
	mset(ins,0,sizeof(tch_rtc_handle_prototype));

	// RTC Clock source enable
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	PWR->CR |= PWR_CR_DBP;   //enable rtc register access


	while(!(RCC->BDCR & RCC_BDCR_LSERDY)) RCC->BDCR |= RCC_BDCR_LSEON;

	// RTC Clock enable
	RCC->BDCR |= ((RTC_CLK_LSE << RTC_CLK_Pos) | RCC_BDCR_RTCEN);

	RTC->WPR = RTC_ACCESS_KEY1;
	RTC->WPR = RTC_ACCESS_KEY2;    // input access-key

	if(RTC->ISR & RTC_ISR_INITS)
	{
		RTC->ISR |= RTC_ISR_INIT;
		while(!(RTC->ISR & RTC_ISR_INITF)) __NOP();

		RTC->PRER = (255 << RTC_PRERS_Pos);  // setup rtc prescaler to obtain 1 Hz
		RTC->PRER |= (127 << RTC_PRERA_Pos);  // setup rtc prescaler to obtain 1 Hz
	}

	cdsl_dlistEntryInit(&ins->alrm_queue);

	ins->mtx = env->Mtx->create();
	ins->pix.close = tch_rtc_close;
	ins->pix.disablePeriodicWakeup = tch_rtc_disablePeriodicWakeup;
	ins->pix.enablePeriodicWakeup = tch_rtc_enablePeriodicWakeup;
	ins->pix.getTime = tch_rtc_getTime;
	ins->pix.setTime = tch_rtc_setTime;

	ins->env = env;
	tch_rtcValidate(ins);

	tch_rtc_setTime((tch_rtcHandle*) ins,ltm,TRUE);
	RTC->ISR &= ~RTC_ISR_INIT;
	return (tch_rtcHandle*) ins;
}

static tchStatus tch_rtc_close(tch_rtcHandle* self)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	tch_rtc_disablePeriodicWakeup(self);

	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return tchErrorResource;
	tch_rtcInvalidate(ins);
	ins->env->Mtx->destroy(ins->mtx);

	if(ins->env->Mtx->lock(&mtx,tchWaitForever) != tchOK)
		return tchErrorResource;

	tch_disableInterrupt(RTC_WKUP_IRQn);
	tch_disableInterrupt(RTC_Alarm_IRQn);

	RCC->BDCR |= RCC_BDCR_BDRST;
	RCC->BDCR &= RCC_BDCR_BDRST;
	RCC->BDCR &= ~RCC_BDCR_RTCEN;

	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;
	RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;
	_handle = 0;
	ins->env->Condv->wakeAll(&condv);
	ins->env->Mtx->unlock(&mtx);
	ins->env->Mem->free(ins);
	return tchOK;
}

static tchStatus tch_rtc_setTime(tch_rtcHandle* self,struct tm* ltm,BOOL force)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	uint32_t dr = 0;
	uint32_t tr = 0;
	uint32_t tmp = 0;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;
	if((RTC->ISR & RTC_ISR_INITS) || !force)   // RTC is already initialized or not forced, it'll not be updated
		return tchOK;
	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return tchErrorResource;
	RTC->ISR |= RTC_ISR_INIT;
	while(!(RTC->ISR & RTC_ISR_INITF))
		/*NOP*/;
	RTC->TR = 0;
	RTC->DR = 0;
	tmp =  ltm->tm_hour / 10;
	tr |= ((tmp << 20) | ((ltm->tm_hour - (tmp * 10)) << 16));
	tmp = ltm->tm_min / 10;
	tr |= ((tmp << 12) | ((ltm->tm_min - (tmp * 10)) << 8));
	tmp = ltm->tm_sec / 10;
	tr |= ((tmp << 4) | (ltm->tm_sec - (tmp * 10)));
	RTC->TR = tr;

	tmp = ltm->tm_year / 10;
	dr |= ((tmp << 20) | ((ltm->tm_year - (tmp * 10)) << 16));
	tmp = ltm->tm_wday == 0? 7 : ltm->tm_wday;
	dr |= (tmp << 13);
	tmp = (ltm->tm_mon + 1) / 10;
	dr |= ((tmp << 12) | (((ltm->tm_mon + 1) - (tmp * 10)) << 8));
	tmp = ltm->tm_mday / 10;
	dr |= ((tmp << 4) | (ltm->tm_mday - (tmp * 10)));
	RTC->DR = dr;


	RTC->PRER = (255 << RTC_PRERS_Pos);  // setup rtc prescaler to obtain 1 Hz
	RTC->PRER |= (127 << RTC_PRERA_Pos);  // setup rtc prescaler to obtain 1 Hz


	RTC->ISR &= ~RTC_ISR_INIT;

	if(ins->env->Mtx->unlock(ins->mtx) != tchOK)
		return tchErrorResource;
	return tchOK;
}

static tchStatus tch_rtc_getTime(tch_rtcHandle* self,struct tm* ltm)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	tchStatus result;
	uint32_t date, time = 0;
	time_t lt;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	if((result = ins->env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
		return result;

	date = RTC->DR;
	time = RTC->TR;

	RTC->TR &= ~RTC_TR_PM;
	mset(ltm,0,sizeof(struct tm));

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

	if(time & RTC_TR_PM)
		RTC->TR |= RTC_TR_PM;
	ins->env->Mtx->unlock(ins->mtx);
	return tchOK;
}


static tchStatus tch_rtc_enablePeriodicWakeup(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler wkup_handler)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;
	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
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
	tch_enableInterrupt(RTC_WKUP_IRQn, PRIORITY_2,NULL);

	return ins->env->Mtx->unlock(ins->mtx);

}

static tchStatus tch_rtc_disablePeriodicWakeup(tch_rtcHandle* self)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*) self;
	if(!ins)
		return tchErrorParameter;
	if(!tch_rtcIsValid(ins))
		return tchErrorParameter;

	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return tchErrorResource;

	RTC->CR &= ~7; // clear wuclk
	RTC->CR |= 0;  // set rtc / 16 : 2048 Hz
	RTC->CR &= ~RTC_CR_WUTE;
	RTC->CR &= ~RTC_CR_WUTIE;
	EXTI->IMR &= ~(1 << 22);
	EXTI->RTSR &= ~(1 << 22);
	ins->wkup_handler = NULL;

	tch_disableInterrupt(RTC_WKUP_IRQn);
	return ins->env->Mtx->unlock(ins->mtx);

}


void RTC_Alarm_IRQHandler()
{

}

void RTC_WKUP_IRQHandler()
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)_handle;
	if(!ins)
		return;
	if(!tch_rtcIsValid(ins))
		return;
	if(ins->wkup_handler)
		ins->wkup_handler();
	EXTI->PR |= (1 << 22);
	RTC->ISR &= ~RTC_ISR_WUTF;
	PWR->CR |= PWR_CR_CWUF;
}
