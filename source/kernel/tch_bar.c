/*
 * tch_bar.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */




/*!\brief Barrier Implementation
 *
 *
 */
#include "tch.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_bar.h"
#include "kernel/tch_sched.h"
#include "kernel/tch_err.h"
#include "kernel/tch_kobj.h"

#ifndef BARRIER_CLASS_KEY
#define BARRIER_CLASS_KEY   	     ((uhword_t) 0x2D03)
#error "might not configured properly"
#endif

#define BAR_VALIDATE(bar)			do {\
	((tch_barCb*) bar)->status |= (((uint32_t) bar ^ BARRIER_CLASS_KEY) & 0xFFFF);\
}while(0)

#define BAR_INVALIDATE(bar)			do {\
	((tch_barCb*) bar)->status &= ~0xFFFF;\
}while(0)

#define BAR_ISVALID(bar)			((((tch_barCb*) bar)->status & 0xFFFF) == (((uint32_t) bar ^ BARRIER_CLASS_KEY) & 0xFFFF))

#define BAR_SET_MAX(bar,max)		do {\
	((tch_barCb*) bar)->status |= ((0xff & max) << 16);\
}while(0)

#define BAR_GET_MAX(bar)			(0xff & (((tch_barCb*) bar)->status >> 16))


__USER_API__ static tch_barId tch_barCreate(uint8_t th_cnt);
__USER_API__ static tchStatus tch_barEnter(tch_barId bar,uint32_t timeout);
__USER_API__ static tchStatus tch_barDestroy(tch_barId bar);

static tch_barId bar_init(tch_barCb* bar,uint8_t cnt,BOOL is_static);
static tchStatus bar_deinit(tch_barCb* bar);

__USER_RODATA__ tch_barrier_api_t Barrier_IX = {
		.create = tch_barCreate,
		.enter = tch_barEnter,
		.destroy = tch_barDestroy
};

__USER_RODATA__ const tch_barrier_api_t* Barrier = &Barrier_IX;


DECLARE_SYSCALL_1(bar_create,uint8_t,tch_barId);
DECLARE_SYSCALL_2(bar_enter,tch_barId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(bar_destroy,tch_barId,tchStatus);
DECLARE_SYSCALL_2(bar_init,tch_barCb*,uint8_t,tchStatus);
DECLARE_SYSCALL_1(bar_deinit,tch_barCb*,tchStatus);


DEFINE_SYSCALL_1(bar_create,uint8_t,th_cnt,tch_barId)
{
	tch_barCb* bar = (tch_barCb*) kmalloc(sizeof(tch_barCb));
	if(!bar)
		KERNEL_PANIC("can't allocate barrier");
	bar_init(bar,th_cnt,FALSE);
	return (tch_barId) bar;
}

DEFINE_SYSCALL_2(bar_enter,tch_barId,bar,uint32_t,timeout,tchStatus)
{
	if((!bar) || (!BAR_ISVALID(bar)))
		return tchErrorParameter;
	uint8_t th_max = BAR_GET_MAX(bar);
	tch_barCb* bcb = (tch_barCb*) bar;
	bcb->th_cnt++;
	if(th_max > bcb->th_cnt)
	{
		tch_schedWait((tch_thread_queue*)&bcb->wq,timeout);
	}
	else
	{
		bcb->th_cnt = 0;
		tch_schedWake((tch_thread_queue*)&bcb->wq,SCHED_THREAD_ALL,tchOK,FALSE);
	}
	return tchOK;
}

DEFINE_SYSCALL_1(bar_destroy,tch_barId,barId,tchStatus)
{
	if((!barId) || (!BAR_ISVALID(barId)))
		return tchErrorParameter;
	bar_deinit(barId);
	kfree(barId);
	return tchOK;
}

DEFINE_SYSCALL_2(bar_init,tch_barCb*,bp,uint8_t,cnt,tchStatus)
{
	if(!bp)
		return tchErrorParameter;
	bar_init(bp,cnt,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(bar_deinit,tch_barCb*,bp,tchStatus)
{
	if(!bp || !BAR_ISVALID(bp))
		return tchErrorParameter;
	bar_deinit(bp);
	return tchOK;
}

static tch_barId bar_init(tch_barCb* bar,uint8_t cnt,BOOL is_static){
	mset(bar, 0, sizeof(tch_barCb));
	BAR_VALIDATE(bar);
	BAR_SET_MAX(bar,cnt);
	bar->th_cnt = 0;
	cdsl_dlistInit(&bar->wq);
	tch_registerKobject(&bar->__obj,is_static? (tch_kobjDestr) tch_barDeinit : (tch_kobjDestr) tch_barDestroy);

	return bar;
}

static tchStatus bar_deinit(tch_barCb* bar){
	if((!bar) || (!BAR_ISVALID(bar)))
		return tchErrorParameter;
	BAR_INVALIDATE(bar);
	tch_schedWake((tch_thread_queue*) &bar->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tch_unregisterKobject(&bar->__obj);
	return tchOK;
}


__USER_API__ static tch_barId tch_barCreate(uint8_t cnt)
{
	if(tch_port_isISR())
		return NULL;
	return (tch_barId) __SYSCALL_0(bar_create);
}

__USER_API__ static tchStatus tch_barEnter(tch_barId barid,uint32_t timeout)
{
	if(!barid)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_2(bar_enter,barid,timeout);
}


__USER_API__ static tchStatus tch_barDestroy(tch_barId barId)
{
	if(!barId)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(bar_destroy,barId);
}


tchStatus tch_barInit(tch_barCb* bar,uint8_t th_cnt){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __bar_init(bar,th_cnt);
	return __SYSCALL_2(bar_init,bar,th_cnt);
}

tchStatus tch_barDeinit(tch_barCb* bar){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __bar_deinit(bar);
	return __SYSCALL_1(bar_deinit,bar);
}
