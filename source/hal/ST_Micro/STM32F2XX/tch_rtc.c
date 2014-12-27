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
#include "hal/tch_hal.h"
#include "tch_kernel.h"

#define RTC_ACCESS_KEY1             ((uint8_t) 0xCA)
#define RTC_ACCESS_KEY2             ((uint8_t) 0x53)

#define RTC_CLK_LSE                 ((uint8_t) 1)
#define RTC_CLK_LSI                 ((uint8_t) 2)
#define RTC_CLK_HSE                 ((uint8_t) 3)
#define RTC_CLK_Msk                 ((uint8_t) 3)
#define RTC_CLK_Pos                 ((uint8_t) 8)

#define RTC_PRERA_Pos               ((uint8_t) 16)
#define RTC_PRERS_Pos               ((uint8_t) 0)

typedef struct tch_lld_rtc_prototype {
	tch_lld_rtc                           pix;
	uint8_t                               occp;
	tch_mtxId                             mtx;
	tch_condvId                           condv;
} tch_lld_rtc_prototype;

typedef struct tch_rtc_handle_prototype_t {
	tch_rtcHandle                         pix;
	uint32_t                              status;
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
static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t localtime);
static time_t tch_rtcGetTime(tch_rtcHandle* self);
static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm);
static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm);
static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint32_t periodInSec);
static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self);
static tchStatus tch_rtcWaitUntilWakeup(tch_rtcHandle* self);


static tchStatus tch_rtcConvertEpochToGMT(struct tch_tm* tm,time_t ep_time);



__attribute__((section(".data"))) static tch_lld_rtc_prototype RTC_StaticInstance = {
		{
				tch_rtcOpen
		},
		0,
		NULL,
		NULL
};


const tch_lld_rtc* rtc = (const tch_lld_rtc*) &RTC_StaticInstance;


static tch_rtcHandle* tch_rtcOpen(const tch* env,time_t lt,tch_timezone tz){
	struct tch_tm gmt;
	time_t epoch = lt;

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
	RTC_StaticInstance.occp = 1;
	if(env->Mtx->unlock(RTC_StaticInstance.mtx) != osOK)
		return NULL;

	localtime_r(&epoch,&gmt);

	// RTC Clock source enable
	PWR->CR |= PWR_CR_DBP;   //enable rtc register access

	RCC->BDCR |= RCC_BDCR_LSEON;
	while(!(RCC->BDCR & RCC_BDCR_LSERDY))
		/*NOP*/ ;

	// RTC Clock enable
	RCC->BDCR |= ((RTC_CLK_LSE << RTC_CLK_Pos) | RCC_BDCR_RTCEN);


	RTC->WPR = RTC_ACCESS_KEY1;
	RTC->WPR = RTC_ACCESS_KEY2;    // input access-key


	RTC->PRER = ((128 << RTC_PRERA_Pos) | (256 << RTC_PRERS_Pos));  // setup rtc prescaler to obtain 1 Hz


//	tch_rtcConvertEpochToGMT(&gmt,tch_systimeTick);

}

static tchStatus tch_rtcClose(tch_rtcHandle* self){

}

static tchStatus tch_rtcSetTime(tch_rtcHandle* self,time_t localtime){

}

static time_t tch_rtcGetTime(tch_rtcHandle* self){

}

static tch_alrId tch_rtcSetAlarm(tch_rtcHandle* self,time_t alrtm){

}

static tchStatus tch_rtcCancelAlarm(tch_rtcHandle* self,tch_alrId alrm){

}

static tchStatus tch_rtcEnablePeriodicWakeup(tch_rtcHandle* self,uint32_t periodInSec){

}

static tchStatus tch_rtcDisablePeriodicWakeup(tch_rtcHandle* self){

}

static tchStatus tch_rtcWaitUntilWakeup(tch_rtcHandle* self){

}


static tchStatus tch_rtcConvertEpochToGMT(struct tch_tm* tm,time_t ep_time){
	ep_time = ep_time / 1000;
}


void RTC_Alarm_IRQHandler(){

}

void RTC_WKUP_IRQHandler(){

}
