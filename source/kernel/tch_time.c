/*
 * tch_timer.c
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
#include "cdsl_dlist.h"

static tch_rtcHandle* rtcHandle;
static cdsl_dlistNode_t tch_systimeWaitQ;
static cdsl_dlistNode_t tch_lpsystimeWaitQ;

volatile uint64_t tch_sysUpTimeSec;
volatile uint64_t tch_systimeTick;

static tchStatus tch_systime_getLocalTime(struct tm* dest_tm);
static tchStatus tch_systime_setLocalTime(struct tm* time, tch_timezone ltz);
static uint64_t tch_systime_getCurrentTimeMills();
static uint64_t tch_systime_uptimeMills();

static DECLARE_COMPARE_FN(tch_systimeWaitQRule);

__attribute__((section(".data")))  static tch_systime_ix TIMER_StaticInstance = {
		tch_systime_getLocalTime,
		tch_systime_setLocalTime,
		tch_systime_getCurrentTimeMills,
		tch_systime_uptimeMills
};

tch_systime_ix* tchk_systimeInit(const tch* env, time_t init_tm,
		tch_timezone init_tz) {

	tch_hal_disableSystick();
	cdsl_dlistInit(&tch_systimeWaitQ);
	cdsl_dlistInit(&tch_lpsystimeWaitQ);
	tch_systimeTick = 0;
	tch_sysUpTimeSec = 0;

	rtcHandle = RTC_IX->open(env, init_tm, init_tz);
	rtcHandle->enablePeriodicWakeup(rtcHandle, 1, tch_kernelOnWakeup);

	tch_hal_enableSystick();

	return &TIMER_StaticInstance;
}

tchStatus tch_systimeSetTimeout(tch_threadId thread, uint32_t timeout,
		tch_timeunit tu) {
	if ((timeout == tchWaitForever) || !thread)
		return tchErrorParameter;
	switch (tu) {
	case mSECOND:
		getThreadKHeader(thread)->to = tch_systimeTick + timeout;
		cdsl_dlistEnqueuePriority(&tch_systimeWaitQ, (cdsl_dlistNode_t*) getThreadKHeader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	case SECOND:
		getThreadKHeader(thread)->to = tch_sysUpTimeSec + timeout;
		cdsl_dlistEnqueuePriority(&tch_lpsystimeWaitQ, (cdsl_dlistNode_t*) getThreadKHeader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	}
	return tchErrorParameter;

}

tchStatus tch_systimeCancelTimeout(tch_threadId thread) {
	if (!thread)
		return tchErrorParameter;
	getThreadKHeader(thread)->to = 0;
	if (!cdsl_dlistRemove( (cdsl_dlistNode_t*) getThreadKHeader(thread)))
		return tchErrorParameter;
	return tchOK;
}

static tchStatus tch_systime_getLocalTime(struct tm* dest_tm) {
	if (!rtcHandle)
		return tchErrorResource;
	if (!dest_tm)
		return tchErrorParameter;
	return rtcHandle->getTime(rtcHandle, dest_tm);
}

static tchStatus tch_systime_setLocalTime(struct tm* time, tch_timezone ltz) {
	time_t tm;
	if (!rtcHandle)
		return tchErrorResource;
	if (!time)
		return tchErrorParameter;
	tm = mktime(time);
	return rtcHandle->setTime(rtcHandle, tm, ltz, TRUE);
}

static uint64_t tch_systime_getCurrentTimeMills() {

}

static uint64_t tch_systime_uptimeMills() {

}

BOOL tch_systimeIsPendingEmpty() {
	return cdsl_dlistIsEmpty(&tch_systimeWaitQ);
}

void tch_kernelOnWakeup() {
	tch_thread_kheader* nth = NULL;
	tch_sysUpTimeSec++;
	while ((!cdsl_dlistIsEmpty(&tch_lpsystimeWaitQ)) && (((tch_thread_kheader*) tch_lpsystimeWaitQ.next)->to	<= tch_sysUpTimeSec)) {
		nth = (tch_thread_kheader*) cdsl_dlistDequeue(&tch_lpsystimeWaitQ);
		nth->to = 0;
		tch_schedReady(nth->uthread);
		tchk_kernelSetResult(nth->uthread, tchEventTimeout);
		if (nth->t_waitQ) {
			cdsl_dlistRemove(&nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedUpdate();
}

void tch_KernelOnSystick() {
	tch_thread_kheader* nth = NULL;
	tch_systimeTick++;
	getThreadKHeader(current)->tslot++;
	while ((!cdsl_dlistIsEmpty(&tch_systimeWaitQ)) && (((tch_thread_kheader*) tch_systimeWaitQ.next)->to <= tch_systimeTick)) {
		nth = (tch_thread_kheader*) cdsl_dlistDequeue(&tch_systimeWaitQ);
		nth->to = 0;
		tch_schedReady(nth->uthread);
		tchk_kernelSetResult(nth->uthread, tchEventTimeout);
		if (nth->t_waitQ) {
			cdsl_dlistRemove(&nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedUpdate();
}

static DECLARE_COMPARE_FN(tch_systimeWaitQRule) {
	return ((tch_thread_kheader*)a)->to < ((tch_thread_kheader*) b)->to? a : b;
}

