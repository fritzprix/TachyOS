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
static tch_lnode tch_systimeWaitQ;
static tch_lnode tch_lpsystimeWaitQ;

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
	tch_listInit(&tch_systimeWaitQ);
	tch_listInit(&tch_lpsystimeWaitQ);
	tch_systimeTick = 0;
	tch_sysUpTimeSec = 0;

	rtcHandle = tch_rtc->open(env, init_tm, init_tz);
	rtcHandle->enablePeriodicWakeup(rtcHandle, 1, tch_kernelOnWakeup);

	tch_hal_enableSystick();

	return &TIMER_StaticInstance;
}

tchStatus tchk_systimeSetTimeout(tch_threadId thread, uint32_t timeout,
		tch_timeunit tu) {
	if ((timeout == tchWaitForever) || !thread)
		return tchErrorParameter;
	switch (tu) {
	case mSECOND:
		getThreadKHeader(thread)->t_to = tch_systimeTick + timeout;
		tch_listEnqueueWithPriority(&tch_systimeWaitQ, (tch_lnode*) getThreadKHeader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	case SECOND:
		getThreadKHeader(thread)->t_to = tch_sysUpTimeSec + timeout;
		tch_listEnqueueWithPriority(&tch_lpsystimeWaitQ, (tch_lnode*) getThreadKHeader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	}
	return tchErrorParameter;

}

tchStatus tch_systimeCancelTimeout(tch_threadId thread) {
	if (!thread)
		return tchErrorParameter;
	getThreadKHeader(thread)->t_to = 0;
	if (!tch_listRemove(&tch_systimeWaitQ, (tch_lnode*) getThreadKHeader(thread)))
		if (!tch_listRemove(&tch_lpsystimeWaitQ, (tch_lnode*) getThreadKHeader(thread)))
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
	return tch_listIsEmpty(&tch_systimeWaitQ);
}

void tch_kernelOnWakeup() {
	tch_thread_kheader* nth = NULL;
	tch_sysUpTimeSec++;
	while ((!tch_listIsEmpty(&tch_lpsystimeWaitQ)) && (((tch_thread_kheader*) tch_lpsystimeWaitQ.next)->t_to	<= tch_sysUpTimeSec)) {
		nth = (tch_thread_kheader*) tch_listDequeue(&tch_lpsystimeWaitQ);
		nth->t_to = 0;
		tchk_schedThreadReady(nth->t_uthread);
		tchk_kernelSetResult(nth->t_uthread, tchEventTimeout);
		if (nth->t_waitQ) {
			tch_listRemove(nth->t_waitQ, &nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedThreadUpdate();
}

void tch_KernelOnSystick() {
	tch_thread_kheader* nth = NULL;
	tch_systimeTick++;
	getThreadKHeader(tch_currentThread)->t_tslot++;
	while ((!tch_listIsEmpty(&tch_systimeWaitQ)) && (((tch_thread_kheader*) tch_systimeWaitQ.next)->t_to <= tch_systimeTick)) {
		nth = (tch_thread_kheader*) tch_listDequeue(&tch_systimeWaitQ);
		nth->t_to = 0;
		tchk_schedThreadReady(nth->t_uthread);
		tchk_kernelSetResult(nth->t_uthread, tchEventTimeout);
		if (nth->t_waitQ) {
			tch_listRemove(nth->t_waitQ, &nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedThreadUpdate();
}

static DECLARE_COMPARE_FN(tch_systimeWaitQRule) {
	return ((tch_thread_kheader*)prior)->t_to < ((tch_thread_kheader*) post)->t_to? prior : post;
}

