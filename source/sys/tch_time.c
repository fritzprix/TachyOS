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

#if !defined(__BUILD_TIME_EPOCH)
#define __BUILD_TIME_EPOCH 0UL
#endif

static tch_rtcHandle* rtcHandle;
static tch_lnode_t tch_systimeWaitQ;
volatile uint64_t tch_systimeTick;



static uint64_t tch_systime_setCurrentTimeMills(uint64_t time);
static uint64_t tch_systime_getCurrentTimeMills();
static uint64_t tch_systime_currentThreadTimeMills();
static uint64_t tch_systime_elapsedRealtime();
static uint64_t tch_systime_uptimeMills();


static DECLARE_COMPARE_FN(tch_systimeWaitQRule);

__attribute__((section(".data"))) static tch_systime_ix TIMER_StaticInstance = {
		tch_systime_setCurrentTimeMills,
		tch_systime_getCurrentTimeMills,
		tch_systime_currentThreadTimeMills,
		tch_systime_elapsedRealtime,
		tch_systime_uptimeMills
};


tch_systime_ix* tch_systimeInit(const tch* env,uint64_t timeInMills){

	tch_hal_disableSystick();
	tch_listInit(&tch_systimeWaitQ);
	tch_systimeTick = __BUILD_TIME_EPOCH * 1000;
	rtcHandle = rtc->open(env,__BUILD_TIME_EPOCH,UTC_P9);

	tch_hal_enableSystick();
	return &TIMER_StaticInstance;
}

tchStatus tch_systimeSetTimeout(tch_threadId thread,uint32_t timeout){
	if((timeout == osWaitForever) || !thread)
		return osErrorParameter;
	getThreadHeader(thread)->t_to = tch_systimeTick + timeout;
	tch_listEnqueuePriority(&tch_systimeWaitQ,(tch_lnode_t*) thread,tch_systimeWaitQRule);
	return osOK;
}

tchStatus tch_systimeCancelTimeout(tch_threadId thread){
	if(!thread)
		return osErrorParameter;
	getThreadHeader(thread)->t_to = 0;
	tch_listRemove(&tch_systimeWaitQ,(tch_lnode_t*) thread);
	return osOK;
}



static uint64_t tch_systime_setCurrentTimeMills(uint64_t time){

}

static uint64_t tch_systime_getCurrentTimeMills(){

}

static uint64_t tch_systime_currentThreadTimeMills(){

}

static uint64_t tch_systime_elapsedRealtime(){

}

static uint64_t tch_systime_uptimeMills(){

}

BOOL tch_systimeHasPending(){
	return tch_listIsEmpty(&tch_systimeWaitQ);
}

void tch_systimeTickHandler(){
	tch_thread_header* nth = NULL;
	tch_systimeTick++;
	tch_currentThread->t_tslot++;
	while((!tch_listIsEmpty(&tch_systimeWaitQ)) && (getThreadHeader(tch_systimeWaitQ.next)->t_to <= tch_systimeTick)){
		nth = (tch_thread_header*) tch_listDequeue(&tch_systimeWaitQ);
		nth->t_to = 0;
		tch_schedReadyThread(nth);
		tch_kernelSetResult(nth,osEventTimeout);
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

