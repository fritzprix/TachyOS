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




static tch_rtcHandle* rtcHandle;
static cdsl_dlistNode_t systimeWaitQ;
static cdsl_dlistNode_t lpsystimeWaitQ;
static tch_timezone current_tz;

volatile uint64_t sysUpTimeSec;		/// time in second since system timer initialized
volatile uint64_t systimeTick;		/// time in millisecond since system timer initialized

volatile uint64_t gmt_sec;			/// without time zone
volatile uint32_t gmt_subsec;		/// millisec


/**
 *  user interface declaration (user accessible code)
 */
__USER_API__ static struct tm* tch_systime_toBrokenTime(uint64_t ms,struct tm* result);
__USER_API__ static tchStatus tch_systime_getLocalTime(struct tm* dest_tm);
__USER_API__ static tchStatus tch_systime_setLocalTime(struct tm* time, tch_timezone ltz);
__USER_API__ static tch_timezone tch_systime_setTimezone(const tch_timezone tz);
__USER_API__ static tch_timezone tch_systime_getTimezone(void);
__USER_API__ static uint64_t tch_systime_getCurrentTimeMills(void);
__USER_API__ static uint64_t tch_systime_uptimeMills(void);


/**
 *  system call declaration
 */
DECLARE_SYSCALL_1(systime_get_local_time,struct tm*,tchStatus);
DECLARE_SYSCALL_2(systime_set_local_time,struct tm*,tch_timezone,tchStatus);
DECLARE_SYSCALL_1(systime_set_timezone,const tch_timezone,tch_timezone);
DECLARE_SYSCALL_0(systime_get_timezone,tch_timezone);
DECLARE_SYSCALL_0(systime_get_current_time_mills,uint64_t);
DECLARE_SYSCALL_0(systime_uptime_mills,uint64_t);


/**
 *  private method declaration
 */
static DECLARE_COMPARE_FN(tch_systimeWaitQRule);
static struct tm* to_broken_time(const time_t* timep,struct tm* result,tch_timezone tz);
static time_t to_gmt_epoch(struct tm* timep);



__USER_RODATA__  tch_kernel_service_time Time_IX = {
		.getLocaltime = tch_systime_getLocalTime,
		.setLocaltime = tch_systime_setLocalTime,
		.setTimezone = tch_systime_setTimezone,
		.getTimezone = tch_systime_getTimezone,
		.getCurrentTimeMills = tch_systime_getCurrentTimeMills,
		.uptimeMills = tch_systime_uptimeMills,
		.toBrokentime = tch_systime_toBrokenTime
};

__USER_RODATA__ const tch_kernel_service_time* Time = &Time_IX;


DEFINE_SYSCALL_1(systime_get_local_time,struct tm*, time, tchStatus){
	if(!rtcHandle)
		return tchErrorResource;

}

DEFINE_SYSCALL_2(systime_set_local_time,struct tm*,ltm,tch_timezone,ltz,tchStatus){

}

DEFINE_SYSCALL_1(systime_set_timezone,const tch_timezone,tz,tch_timezone){

}

DEFINE_SYSCALL_0(systime_get_timezone,tch_timezone){

}

DEFINE_SYSCALL_0(systime_get_current_time_mills,uint64_t){

}

DEFINE_SYSCALL_0(systime_uptime_mills,uint64_t){

}



void tch_systimeInit(const tch* env, time_t init_tm, tch_timezone init_tz) {

	tch_hal_disableSystick();
	cdsl_dlistInit(&systimeWaitQ);
	cdsl_dlistInit(&lpsystimeWaitQ);
	systimeTick = 0;
	sysUpTimeSec = 0;
	current_tz = init_tz;
	tch_device_service_rtc* rtc = (tch_device_service_rtc*) tch_kmod_request(MODULE_TYPE_RTC);
	if(!rtc){
		KERNEL_PANIC("tch_time.c","rtc is not available");
	}

	gmt_sec = init_tm;
	gmt_subsec = 0;
	rtcHandle = rtc->open(env, init_tm, current_tz);
	rtcHandle->enablePeriodicWakeup(rtcHandle, LSTICK_PERIOD, tch_kernel_onWakeup);
	tch_hal_enableSystick(HSTICK_PERIOD);
}


tch_timezone tch_systimeSetTimeZone(tch_timezone itz){

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

static struct tm* tch_systime_toBrokenTime(uint64_t ms,struct tm* result){
	return result;
}


static tchStatus tch_systime_getLocalTime(struct tm* dest_tm) {
	if(tch_port_isISR())
	{
		return __systime_get_local_time(dest_tm);
	}
	return __SYSCALL_1(systime_get_local_time,dest_tm);
}

static tchStatus tch_systime_setLocalTime(struct tm* time, tch_timezone ltz) {
	time_t tm;
	if (!rtcHandle)
		return tchErrorResource;
	if (!time)
		return tchErrorParameter;
	tm = tch_time_broken_to_gmt_epoch(time);
	return rtcHandle->setTime(rtcHandle, tm, ltz, TRUE);
}


static tch_timezone tch_systime_setTimezone(const tch_timezone tz){

}

static tch_timezone tch_systime_getTimezone(){

}

static uint64_t tch_systime_getCurrentTimeMills() {

}

static uint64_t tch_systime_uptimeMills() {

}

BOOL tch_systimeIsPendingEmpty() {
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

