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



static tch_rtcHandle* tch_rtc_open(const tch_core_api_t* env, struct tm* localtm);
static tchStatus tch_rtc_close(tch_rtcHandle* self);
static tchStatus tch_rtc_setTime(tch_rtcHandle* self, struct tm* localtm, BOOL force, tch_timezone zone);
static tchStatus tch_rtc_getTime(tch_rtcHandle* self, struct tm* localtm, tch_timezone zone);
static tchStatus tch_rtc_enablePeriodicWakeup(tch_rtcHandle* self, uint16_t periodInSec, tch_rtc_wkupHandler handler);
static tchStatus tch_rtc_disablePeriodicWakeup(tch_rtcHandle* self);

static void inline rtc_configuration_begin();
static void inline rtc_configuration_end();
static time_t inline read_current_time();

__USER_RODATA__ tch_hal_module_rtc_t RTC_Ops ={
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
	return tch_kmod_register(MODULE_TYPE_RTC, RTC_CLASS_KEY, &RTC_Ops, TRUE);
}

static void tch_rtc_exit(void)
{
	RCC->APB1ENR &= ~RCC_APB1ENR_PWREN;

	tch_mutexDeinit(&mtx);
	tch_condvDeinit(&condv);
	tch_kmod_unregister(MODULE_TYPE_RTC, RTC_CLASS_KEY);
}

MODULE_INIT(tch_rtc_init);
MODULE_EXIT(tch_rtc_exit);

static void inline rtc_configuration_begin()
{
	while (!(RTC->CRL & RTC_CRL_RTOFF)) __NOP();
	RTC->CRL |= RTC_CRL_CNF;
}

static void inline rtc_configuration_end()
{
	RTC->CRL &= ~RTC_CRL_CNF;
	while (!(RTC->CRL & RTC_CRL_RTOFF)) __NOP();
}


static tch_rtcHandle* tch_rtc_open(const tch_core_api_t* env, struct tm* ltm)
{
	tch_rtc_handle_prototype* ins = NULL;
	if (env->Mtx->lock(&mtx, tchWaitForever) != tchOK)
	{
		return NULL;
	}

	while (_handle)
	{
		if (env->Condv->wait(&condv, &mtx, tchWaitForever) != tchOK)
			return NULL;
	}


	_handle = ins = (tch_rtc_handle_prototype*)env->Mem->alloc(sizeof(tch_rtc_handle_prototype));
	if (env->Mtx->unlock(&mtx) != tchOK)
	{
		return NULL;
	}

	mset(ins, 0, sizeof(tch_rtc_handle_prototype));
	// RTC Clock source enable
	RCC->APB1ENR |= (RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN);
	PWR->CR |= PWR_CR_DBP;   //enable rtc register access


	while (!(RCC->BDCR & RCC_BDCR_LSERDY)) RCC->BDCR |= RCC_BDCR_LSEON;

	// RTC Clock enable
	RCC->BDCR |= ((RTC_CLK_LSE << RTC_CLK_Pos) | RCC_BDCR_RTCEN);

	// Put RTC in configuration mode
	rtc_configuration_begin();

	RTC->PRLL = 0xFFFF; ///  1 hz  (32.768 / (1 + PRL))

	cdsl_dlistEntryInit(&ins->alrm_queue);

	ins->mtx = env->Mtx->create();
	ins->pix.close = tch_rtc_close;
	ins->pix.disablePeriodicWakeup = tch_rtc_disablePeriodicWakeup;
	ins->pix.enablePeriodicWakeup = tch_rtc_enablePeriodicWakeup;
	ins->pix.getTime = tch_rtc_getTime;
	ins->pix.setTime = tch_rtc_setTime;

	ins->env = env;
	tch_rtcValidate(ins);

	tch_rtc_setTime((tch_rtcHandle*)ins, ltm, TRUE, UTC_P9);
	rtc_configuration_end();

	return (tch_rtcHandle*)ins;
}

static tchStatus tch_rtc_close(tch_rtcHandle* self)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)self;
	if (!ins)
		return tchErrorParameter;
	if (!tch_rtcIsValid(ins))
		return tchErrorParameter;

	tch_rtc_disablePeriodicWakeup(self);

	if (ins->env->Mtx->lock(ins->mtx, tchWaitForever) != tchOK)
		return tchErrorResource;
	tch_rtcInvalidate(ins);
	ins->env->Mtx->destroy(ins->mtx);

	if (ins->env->Mtx->lock(&mtx, tchWaitForever) != tchOK)
		return tchErrorResource;

	tch_disableInterrupt(RTC_IRQn);

	RCC->BDCR |= RCC_BDCR_BDRST;
	RCC->BDCR &= ~RCC_BDCR_BDRST;
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

static tchStatus tch_rtc_setTime(tch_rtcHandle* self, struct tm* ltm, BOOL force, tch_timezone zone)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)self;
	uint32_t dr = 0;
	uint32_t tr = 0;
	uint32_t tmp = 0;
	if (!ins)
	{
		return tchErrorParameter;
	}
	if (!tch_rtcIsValid(ins))
	{
		return tchErrorParameter;
	}

	if (ins->env->Mtx->lock(ins->mtx, tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}

	rtc_configuration_begin();
	time_t tm = tch_time_broken_to_gmt_epoch(ltm, zone);
	/*NOP*/;
	RTC->CNTL = (tm && 0xFFFF);
	RTC->CNTL = (tm >> 15) && 0xFFFF;

	rtc_configuration_end();

	if (ins->env->Mtx->unlock(ins->mtx) != tchOK)
	{
		return tchErrorResource;
	}
	return tchOK;
}

static tchStatus tch_rtc_getTime(tch_rtcHandle* self, struct tm* ltm, tch_timezone zone)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)self;
	tchStatus result;
	uint32_t date, time = 0;
	time_t lt;
	if (!ins)
	{
		return tchErrorParameter;
	}
	if (!tch_rtcIsValid(ins))
	{
		return tchErrorParameter;
	}

	if ((result = ins->env->Mtx->lock(ins->mtx, tchWaitForever)) != tchOK)
	{
		return result;
	}

	time_t tm = RTC->CNTH;
	tm <<= 15;
	tm |= RTC->CNTL;
	mset(ltm, 0, sizeof(struct tm));
	tch_time_gmt_epoch_to_broken(&tm, ltm, zone);
	ins->env->Mtx->unlock(ins->mtx);
	return tchOK;
}


static tchStatus tch_rtc_enablePeriodicWakeup(tch_rtcHandle* self, uint16_t periodInSec, tch_rtc_wkupHandler wkup_handler)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)self;
	if (!ins)
	{
		return tchErrorParameter;
	}
	if (!tch_rtcIsValid(ins))
	{
		return tchErrorParameter;
	}
	if (ins->env->Mtx->lock(ins->mtx, tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}
	ins->wkup_period = periodInSec;
	/*NOP*/;
	ins->wkup_handler = wkup_handler;
	rtc_configuration_begin();
	RTC->CRH |= RTC_CRH_SECIE;
	rtc_configuration_end();
	tch_enableInterrupt(RTC_IRQn, PRIORITY_2, NULL);
	return ins->env->Mtx->unlock(ins->mtx);
}

static tchStatus tch_rtc_disablePeriodicWakeup(tch_rtcHandle* self)
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)self;
	if (!ins)
	{
		return tchErrorParameter;
	}
	if (!tch_rtcIsValid(ins))
	{
		return tchErrorParameter;
	}

	if (ins->env->Mtx->lock(ins->mtx, tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}

	ins->wkup_handler = NULL;
	rtc_configuration_begin();
	RTC->CRH &= ~RTC_CRH_SECIE;
	rtc_configuration_end();
	tch_disableInterrupt(RTC_IRQn);
	return ins->env->Mtx->unlock(ins->mtx);

}



void RTC_IRQHandler()
{
	tch_rtc_handle_prototype* ins = (tch_rtc_handle_prototype*)_handle;
	if (!ins)
	{
		return;
	}
	if (!tch_rtcIsValid(ins))
	{
		return;
	}
	if (!ins->wkup_handler)
	{
		return;
		
	}

	if(RTC->CRL & RTC_CRL_SECF) 
	{
		time_t ctime = read_current_time();
		if(!ctime % ins->wkup_period) 
		{
			ins->wkup_handler();
		}
	}
	rtc_configuration_end();
	RTC->CRL &= ~RTC_CRL_SECF;
	rtc_configuration_end();
}


static time_t inline read_current_time()
{
	time_t v = RTC->CNTH << 15;
	v |= RTC->CNTL;
	return v;
}
