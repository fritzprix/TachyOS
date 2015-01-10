/*
 * tch_vtimer.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 30.
 *      Author: manics99
 */


#include "tch_kernel.h"
#include "tch_time.h"
#include "tch_list.h"


static tch_rtcHandle* rtcHandle;
static tch_lnode_t tch_systimeWaitQ;
volatile uint64_t tch_systimeTick;
static tch_wkup_handler tch_sysWkuphandler;

/*
 *
	tchStatus (*getLocaltime)(struct tm* tm);
	tchStatus (*setLocaltime)(const struct tm* tm,const tch_timezone tz);
 */

static tchStatus tch_systime_getLocalTime(struct tm* dest_tm);
static tchStatus tch_systime_setLocalTime(struct tm* time,tch_timezone ltz);
static uint64_t tch_systime_getCurrentTimeMills();
static uint64_t tch_systime_uptimeMills();


static DECLARE_COMPARE_FN(tch_systimeWaitQRule);
static DECLARE_RTC_WKUP_FN(tch_systimeWkupHandler);

__attribute__((section(".data"))) static tch_systime_ix TIMER_StaticInstance = {
		tch_systime_getLocalTime,
		tch_systime_setLocalTime,
		tch_systime_getCurrentTimeMills,
		tch_systime_uptimeMills
};


tch_systime_ix* tch_systimeInit(const tch* env,time_t init_tm,tch_timezone init_tz,tch_wkup_handler wkup_handler){

	tch_hal_disableSystick();
	tch_listInit(&tch_systimeWaitQ);
	rtcHandle = tch_rtc->open(env,init_tm,init_tz);
	rtcHandle->enablePeriodicWakeup(rtcHandle,1,wkup_handler);
	tch_sysWkuphandler = wkup_handler;
	tch_systimeTick = 0;

	tch_hal_enableSystick();

	return &TIMER_StaticInstance;
}

tchStatus tch_systimeSetTimeout(tch_threadId thread,uint32_t timeout){
	if((timeout == tchWaitForever) || !thread)
		return tchErrorParameter;
	getThreadHeader(thread)->t_to = tch_systimeTick + timeout;
	tch_listEnqueuePriority(&tch_systimeWaitQ,(tch_lnode_t*) thread,tch_systimeWaitQRule);
	return tchOK;
}

tchStatus tch_systimeCancelTimeout(tch_threadId thread){
	if(!thread)
		return tchErrorParameter;
	getThreadHeader(thread)->t_to = 0;
	tch_listRemove(&tch_systimeWaitQ,(tch_lnode_t*) thread);
	return tchOK;
}

static tchStatus tch_systime_getLocalTime(struct tm* dest_tm){
	if(!rtcHandle)
		return tchErrorResource;
	if(!dest_tm)
		return tchErrorParameter;
	return rtcHandle->getTime(rtcHandle,dest_tm);
}


static tchStatus tch_systime_setLocalTime(struct tm* time,tch_timezone ltz){
	time_t tm;
	if(!rtcHandle)
		return tchErrorResource;
	if(!time)
		return tchErrorParameter;
	tm = mktime(time);
	return rtcHandle->setTime(rtcHandle,tm,ltz,TRUE);
}

static uint64_t tch_systime_getCurrentTimeMills(){

}

static uint64_t tch_systime_uptimeMills(){

}

BOOL tch_systimeIsPendingEmpty(){
	return tch_listIsEmpty(&tch_systimeWaitQ);
}

void tch_KernelOnSystick(){
	tch_thread_header* nth = NULL;
	tch_systimeTick++;
	tch_currentThread->t_tslot++;
	while((!tch_listIsEmpty(&tch_systimeWaitQ)) && (getThreadHeader(tch_systimeWaitQ.next)->t_to <= tch_systimeTick)){
		nth = (tch_thread_header*) tch_listDequeue(&tch_systimeWaitQ);
		nth->t_to = 0;
		tch_schedReadyThread(nth);
		tch_kernelSetResult(nth,tchEventTimeout);
		if(nth->t_waitQ){
			tch_listRemove(nth->t_waitQ,&nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedTryPreemption();
}


static DECLARE_COMPARE_FN(tch_systimeWaitQRule){
	return getThreadHeader(prior)->t_to < getThreadHeader(post)->t_to;
}

static DECLARE_RTC_WKUP_FN(tch_systimeWkupHandler){
	tch_kernelOnWakeup();
}

