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


#define BARRIER_CLASS_KEY   	     ((uhword_t) 0x2D03)

#define BAR_VALIDATE(bar)			do {\
	((tch_barCb*) bar)->status |= (((uint32_t) bar ^ BARRIER_CLASS_KEY) & 0xFFFF);\
}while(0)

#define BAR_INVALIDATE(bar)			do {\
	((tch_barCb*) bar)->status &= ~0xFFFF;\
}while(0)

#define BAR_ISVALID(bar)			((((tch_barCb*) bar)->status & 0xFFFF) == (((uint32_t) bar ^ BARRIER_CLASS_KEY) & 0xFFFF))



__USER_API__ static tch_barId tch_barCreate();
__USER_API__ static tchStatus tch_barWait(tch_barId bar,uint32_t timeout);
__USER_API__ static tchStatus tch_barSignal(tch_barId bar,tchStatus result);
__USER_API__ static tchStatus tch_barDestroy(tch_barId bar);

static tch_barId bar_init(tch_barCb* bar,BOOL is_static);
static tchStatus bar_deinit(tch_barCb* bar);

__USER_RODATA__ tch_kernel_service_barrier Barrier_IX = {
		.create = tch_barCreate,
		.wait = tch_barWait,
		.signal = tch_barSignal,
		.destroy = tch_barDestroy
};

__USER_RODATA__ const tch_kernel_service_barrier* Barrier = &Barrier_IX;


DECLARE_SYSCALL_0(bar_create,tch_barId);
DECLARE_SYSCALL_2(bar_wait,tch_barId,uint32_t,tchStatus);
DECLARE_SYSCALL_2(bar_signal,tch_barId,tchStatus,tchStatus);
DECLARE_SYSCALL_1(bar_destroy,tch_barId,tchStatus);
DECLARE_SYSCALL_1(bar_init,tch_barCb*,tchStatus);
DECLARE_SYSCALL_1(bar_deinit,tch_barCb*,tchStatus);


DEFINE_SYSCALL_0(bar_create,tch_barId) {
	tch_barCb* bar = (tch_barCb*) kmalloc(sizeof(tch_barCb));
	if(!bar)
		KERNEL_PANIC("tch_bar.c","can't allocate barrier");
	bar_init(bar,FALSE);
	return (tch_barId) bar;
}

DEFINE_SYSCALL_2(bar_wait,tch_barId,bar,uint32_t,timeout,tchStatus) {
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	tch_barCb* _bar = (tch_barCb*) bar;
	return tch_schedWait((tch_thread_queue*) &_bar->wq,timeout);
}

DEFINE_SYSCALL_2(bar_signal,tch_barId,barId,tchStatus,result,tchStatus){
	tch_barCb* bar = (tch_barCb*) barId;
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	if(cdsl_dlistIsEmpty(&bar->wq))
		return tchOK;
	tch_schedWake((tch_thread_queue*)&bar->wq,SCHED_THREAD_ALL,tchOK,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(bar_destroy,tch_barId,barId,tchStatus){
	if((!barId) || (!BAR_ISVALID(barId)))
		return tchErrorParameter;
	bar_deinit(barId);
	kfree(barId);
	return tchOK;
}

DEFINE_SYSCALL_1(bar_init,tch_barCb*,bp,tchStatus){
	if(!bp)
		return tchErrorParameter;
	bar_init(bp,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(bar_deinit,tch_barCb*,bp,tchStatus){
	if(!bp || !BAR_ISVALID(bp))
		return tchErrorParameter;
	bar_deinit(bp);
	return tchOK;
}

static tch_barId bar_init(tch_barCb* bar,BOOL is_static){
	mset(bar, 0, sizeof(tch_barCb));
	BAR_VALIDATE(bar);
	cdsl_dlistInit(&bar->wq);
	tch_registerKobject(&bar->__obj,is_static? (tch_kobjDestr) bar_deinit : (tch_kobjDestr) tch_barDestroy);

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


__USER_API__ static tch_barId tch_barCreate(){
	if(tch_port_isISR())
		return NULL;
	return (tch_barId) __SYSCALL_0(bar_create);
}

__USER_API__ static tchStatus tch_barWait(tch_barId bar,uint32_t timeout){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_2(bar_wait,bar,timeout);
}

__USER_API__ static tchStatus tch_barSignal(tch_barId barId,tchStatus result){
	tch_barCb* bar = (tch_barCb*) barId;
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR()){
		return __bar_signal(barId,result);
	}
	return __SYSCALL_2(bar_signal,barId,result);
}

__USER_API__ static tchStatus tch_barDestroy(tch_barId barId){
	if(!barId)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(bar_destroy,barId);
}


tchStatus tch_barInit(tch_barCb* bar){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __bar_init(bar);
	return __SYSCALL_1(bar_init,bar);
}

tchStatus tch_barDeinit(tch_barCb* bar){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __bar_deinit(bar);
	return __SYSCALL_1(bar_deinit,bar);
}
