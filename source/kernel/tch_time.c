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

#include "kernel/tch_kernel.h"
#include "kernel/tch_time.h"
#include "kernel/tch_err.h"
#include "tch_port.h"

#include "kernel/util/cdsl_dlist.h"
#include "kernel/util/time.h"




static cdsl_dlistNode_t systimeWaitQ;
static cdsl_dlistNode_t lpsystimeWaitQ;

__USER_DATA static tch_timezone current_tz;

__USER_DATA volatile uint64_t systimeTick;		/// time in millisecond since system timer initialized
__USER_DATA volatile uint64_t sysUpTimeSec;
__USER_DATA volatile time_t gmt_epoch;			/// without time zone

__USER_DATA static tch_rtcHandle* rtcHandle;


/**
 */

__USER_API__ static tchStatus tch_systime_getWorldTime(time_t* tp);
__USER_API__ static tchStatus tch_systime_setWorldTime(const time_t epoch_gmt);
__USER_API__ static tch_timezone tch_systime_setTimezone(const tch_timezone tz);
__USER_API__ static tch_timezone tch_systime_getTimezone(void);
__USER_API__ static uint64_t tch_systime_getCurrentTimeMills(void);
__USER_API__ static uint64_t tch_systime_uptimeMills(void);
__USER_API__ static time_t tch_systime_fromBrokenTime(struct tm* tp);
__USER_API__ static void tch_systime_fromEpochTime(const time_t time, struct tm* dest_tm,tch_timezone tz);




__USER_RODATA__  tch_kernel_service_time Time_IX = {
		.getWorldTime = tch_systime_getWorldTime,
		.setWorldTime = tch_systime_setWorldTime,
		.setTimezone = tch_systime_setTimezone,
		.getTimezone = tch_systime_getTimezone,
		.getCurrentTimeMills = tch_systime_getCurrentTimeMills,
		.uptimeMills = tch_systime_uptimeMills,
		.fromBrokenTime = tch_systime_fromBrokenTime,
		.fromEpochTime = tch_systime_fromEpochTime
};

__USER_RODATA__ const tch_kernel_service_time* Time = &Time_IX;



void tch_systimeInit(const tch* env, time_t init_tm, tch_timezone init_tz) {

	tch_hal_disableSystick();
	cdsl_dlistInit(&systimeWaitQ);
	cdsl_dlistInit(&lpsystimeWaitQ);
	systimeTick = 0;
	sysUpTimeSec = 0;
	current_tz = init_tz;
	gmt_epoch = init_tm;
	tch_device_service_rtc* rtc = (tch_device_service_rtc*) tch_kmod_request(MODULE_TYPE_RTC);

	if(!rtc)
	{
		KERNEL_PANIC("tch_time.c","rtc is not available");
	}

	struct tm localtm;
	tch_time_gmt_epoch_to_broken(&init_tm,&localtm,init_tz);	// get local time as broken time
	rtcHandle = rtc->open(env, &localtm);
	rtcHandle->enablePeriodicWakeup(rtcHandle, LSTICK_PERIOD, tch_kernel_onWakeup);
	tch_hal_enableSystick(HSTICK_PERIOD);
}


tchStatus tch_systimeSetTimeout(tch_threadId thread, uint32_t timeout,
		tch_timeunit tu) {
	if ((timeout == tchWaitForever) || !thread)
		return tchErrorParameter;
	switch (tu) {
	case mSECOND:
		get_thread_kheader(thread)->to = systimeTick + timeout;
		cdsl_dlistEnqueuePriority(&systimeWaitQ, (cdsl_dlistNode_t*) get_thread_kheader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	case SECOND:
		get_thread_kheader(thread)->to = sysUpTimeSec + timeout;
		cdsl_dlistEnqueuePriority(&lpsystimeWaitQ, (cdsl_dlistNode_t*) get_thread_kheader(thread),
				tch_systimeWaitQRule);
		return tchOK;
	}
	return tchErrorParameter;

}

tchStatus tch_systimeCancelTimeout(tch_threadId thread) {
	if (!thread)
		return tchErrorParameter;
	get_thread_kheader(thread)->to = 0;
	if (!cdsl_dlistRemove( (cdsl_dlistNode_t*) get_thread_kheader(thread)))
		return tchErrorParameter;
	return tchOK;
}

static tchStatus tch_systime_getWorldTime(time_t* tp)
{
	if(!rtcHandle)
		return tchErrorResource;
	if(!tp)
		return tchErrorParameter;
	struct tm ltm;
	rtcHandle->getTime(rtcHandle, &ltm);
	*tp = tch_time_broken_to_gmt_epoch(&ltm,current_tz);
	return tchOK;
}

static tchStatus tch_systime_setWorldTime(const time_t epoch_gmt)
{
	if(!rtcHandle)
		return tchErrorResource;
	struct tm ltm;
	tchStatus res;
	tch_time_gmt_epoch_to_broken(&epoch_gmt,&ltm,current_tz);
	if((res = rtcHandle->setTime(rtcHandle,&ltm,TRUE)) == tchOK)
	{
		gmt_epoch = epoch_gmt;
	}
	return res;
}


static tch_timezone tch_systime_setTimezone(const tch_timezone tz)
{
	tch_timezone prev_tz = current_tz;
	current_tz = tz;
	struct tm ltm;
	tch_time_gmt_epoch_to_broken((time_t*) &gmt_epoch,&ltm,current_tz);
	if(rtcHandle)
	{
		rtcHandle->setTime(rtcHandle,&ltm,TRUE);
	}
	return prev_tz;
}

static tch_timezone tch_systime_getTimezone()
{
	return current_tz;
}

static uint64_t tch_systime_getCurrentTimeMills()
{
	uint64_t world_mill_sec = gmt_epoch * 1000;
	world_mill_sec += systimeTick % 1000;
	return world_mill_sec;
}

static uint64_t tch_systime_uptimeMills()
{
	uint64_t uptime_mill_sec = sysUpTimeSec * 1000;
	uptime_mill_sec += systimeTick % 1000;
	return uptime_mill_sec;
}

static time_t tch_systime_fromBrokenTime(struct tm* tp)
{
	return tch_time_broken_to_gmt_epoch(tp,current_tz);
}

static void tch_systime_fromEpochTime(const time_t time, struct tm* dest_tm,tch_timezone tz)
{
	tch_time_gmt_epoch_to_broken(&time,dest_tm,tz);
}

BOOL tch_systimeIsPendingEmpty()
{
	return cdsl_dlistIsEmpty(&systimeWaitQ);
}

void tch_kernel_onWakeup() {
	tch_thread_kheader* nth = NULL;
	sysUpTimeSec += LSTICK_PERIOD;
	while ((!cdsl_dlistIsEmpty(&lpsystimeWaitQ)) && (((tch_thread_kheader*) lpsystimeWaitQ.next)->to	<= sysUpTimeSec)) {
		nth = (tch_thread_kheader*) cdsl_dlistDequeue(&lpsystimeWaitQ);
		nth->to = 0;
		tch_schedReady(nth->uthread);
		tch_kernel_set_result(nth->uthread, tchEventTimeout);
		if (nth->t_waitQ) {
			cdsl_dlistRemove(&nth->t_waitNode);
			nth->t_waitQ = NULL;
		}
	}
	tch_schedUpdate();
}

void tch_kernel_onSystick() {
	tch_thread_kheader* nth = NULL;
	systimeTick += HSTICK_PERIOD;
	get_thread_kheader(current)->tslot++;
	while ((!cdsl_dlistIsEmpty(&systimeWaitQ)) && (((tch_thread_kheader*) systimeWaitQ.next)->to <= systimeTick)) {
		nth = (tch_thread_kheader*) cdsl_dlistDequeue(&systimeWaitQ);
		nth->to = 0;
		tch_schedReady(nth->uthread);
		tch_kernel_set_result(nth->uthread, tchEventTimeout);
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

