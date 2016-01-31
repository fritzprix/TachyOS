/*
 * tch_waitq.c
 *
 *  Created on: 2016. 1. 31.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_err.h"
#include "tch_kobj.h"
#include "tch_waitq.h"
#include "kernel/util/string.h"

#ifndef WAITQ_CLASS_KEY
#define WAITQ_CLASS_KEY				((uint16_t) 0xF53A)
#error "might not configured properly"
#endif

#define WAITQ_VALIDATE(wq)			do {\
	((tch_waitqCb*) wq)->status |= (((uint32_t) wq ^ WAITQ_CLASS_KEY) & 0xFFFF);\
}while(0)

#define WAITQ_INVALIDATE(wq)			do {\
	((tch_waitqCb*) wq)->status &= ~0xFFFF;\
}while(0)

#define WAITQ_ISVALID(wq)			((((tch_waitqCb*) wq)->status & 0xFFFF) == (((uint32_t) wq ^ WAITQ_CLASS_KEY) & 0xFFFF))

#define WAITQ_SETPOL(wq,pol) 		do {\
	((tch_waitqCb*) wq)->status |= (pol << 16);\
}while(0)

#define WAITQ_GETPOL(wq)			((((tch_waitqCb*) wq)->status >> 16) & 0xffff)


DECLARE_SYSCALL_1(waitq_create,tch_waitqPolicy,tch_waitqId);
DECLARE_SYSCALL_2(waitq_sleep, tch_waitqId, uint32_t,tchStatus);
DECLARE_SYSCALL_2(waitq_wake, tch_waitqId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(waitq_destroy, tch_waitqId,tchStatus);
DECLARE_SYSCALL_3(waitq_init,tch_waitqCb*, tch_waitqPolicy, BOOL, tchStatus);
DECLARE_SYSCALL_1(waitq_deinit,tch_waitqCb*, tchStatus);

__USER_API__ static tch_waitqId tch_waitqCreate(tch_waitqPolicy policy);
__USER_API__ static tchStatus tch_waitqSleep(tch_waitqId waitq,uint32_t timeout);
__USER_API__ static tchStatus tch_waitqWakeup(tch_waitqId waitq);
__USER_API__ static tchStatus tch_waitqWakeupAll(tch_waitqId waitq);
__USER_API__ static tchStatus tch_waitqDestroy(tch_waitqId waitq);

static tchStatus waitq_init(tch_waitqCb* wq,tch_waitqPolicy policy,BOOL is_static);
static tchStatus waitq_deinit(tch_waitqCb* wq);

__USER_RODATA__  tch_waitq_api_t WaitQ_IX = {
		.create = tch_waitqCreate,
		.sleep = tch_waitqSleep,
		.wake = tch_waitqWakeup,
		.wakeAll = tch_waitqWakeupAll,
		.destroy = tch_waitqDestroy
};

__USER_RODATA__ const tch_waitq_api_t* WaitQ = &WaitQ_IX;


DEFINE_SYSCALL_1(waitq_create,tch_waitqPolicy, policy, tch_waitqId)
{
	tch_waitqCb* wq = (tch_waitqCb*) kmalloc(sizeof(tch_waitqCb));
	if(!wq)
		KERNEL_PANIC("waitq can't be created");
	if(waitq_init(wq,policy,FALSE) != tchOK)
	{
		kfree(wq);
		return NULL;
	}
	return (tch_waitqId) wq;
}

DEFINE_SYSCALL_2(waitq_sleep, tch_waitqId, wq,uint32_t,timeout,tchStatus)
{
	if(!wq)
		return tchErrorParameter;
	if(!WAITQ_ISVALID(wq))
		return tchErrorResource;
	tch_waitqCb* wqp = (tch_waitqCb*) wq;
	return tch_schedWait(&wqp->wq, timeout);
}

DEFINE_SYSCALL_2(waitq_wake, tch_waitqId, wq, uint32_t, cnt, tchStatus)
{
	if(!wq)
		return tchErrorParameter;
	if(!WAITQ_ISVALID(wq))
		return tchErrorResource;
	tch_waitqCb* wqp = (tch_waitqCb*) wq;
	tch_schedWake(&wqp->wq,cnt,tchOK,FALSE);
	return TRUE;
}

DEFINE_SYSCALL_1(waitq_destroy, tch_waitqId,wq, tchStatus)
{
	if(!wq || !WAITQ_ISVALID(wq))
		return tchErrorParameter;
	tchStatus res = waitq_deinit(wq);
	kfree(wq);
	return res;
}

DEFINE_SYSCALL_3(waitq_init, tch_waitqCb*, wq,tch_waitqPolicy , pol,BOOL, isstatic,tchStatus)
{
	if(!wq)
		return tchErrorParameter;
	waitq_init(wq,pol,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(waitq_deinit, tch_waitqCb*, wq, tchStatus)
{
	if(!wq || !WAITQ_ISVALID(wq))
		return tchErrorParameter;
	return waitq_deinit(wq);
}


static tchStatus waitq_init(tch_waitqCb* wq, tch_waitqPolicy policy, BOOL is_static)
{
	if(!wq)
		return tchErrorParameter;
	mset(wq, 0 ,sizeof(tch_waitqCb));
	cdsl_dlistInit((cdsl_dlistNode_t*) &wq->wq);
	WAITQ_SETPOL(wq, policy);
	tch_registerKobject(&wq->__kobj,is_static? (tch_kobjDestr) tch_waitqDeinit : (tch_kobjDestr) tch_waitqDestroy);
	WAITQ_VALIDATE(wq);
	return tchOK;
}

static tchStatus waitq_deinit(tch_waitqCb* wq)
{
	if(!wq || !WAITQ_ISVALID(wq))
		return tchErrorParameter;
	WAITQ_INVALIDATE(wq);
	tch_unregisterKobject(&wq->__kobj);
	tch_schedWake(&wq->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	return tchOK;
}


__USER_API__ static tch_waitqId tch_waitqCreate(tch_waitqPolicy policy)
{
	if(tch_port_isISR())
		return NULL;
	return (tch_waitqId) __SYSCALL_1(waitq_create, policy);
}

__USER_API__ static tchStatus tch_waitqSleep(tch_waitqId waitq, uint32_t timeout)
{
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_2(waitq_sleep,waitq,timeout);
}

__USER_API__ static tchStatus tch_waitqWakeup(tch_waitqId waitq)
{
	if(tch_port_isISR())
		return __waitq_wake(waitq, 1);
	return __SYSCALL_2(waitq_wake, waitq, 1);
}

__USER_API__ static tchStatus tch_waitqWakeupAll(tch_waitqId waitq)
{
	if(tch_port_isISR())
		return __waitq_wake(waitq, SCHED_THREAD_ALL);
	return __SYSCALL_2(waitq_wake, waitq, SCHED_THREAD_ALL);
}

__USER_API__ static tchStatus tch_waitqDestroy(tch_waitqId waitq)
{
	if(!waitq)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(waitq_destroy,waitq);
}


tchStatus tch_waitqInit(tch_waitqCb* waitqp, tch_waitqPolicy policy)
{
	if(!waitqp)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __waitq_init(waitqp, policy, TRUE);
	return  __SYSCALL_3(waitq_init,waitqp, policy ,TRUE);
}

tchStatus tch_waitqDeinit(tch_waitqCb* waitqp)
{
	if(!waitqp)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __waitq_deinit(waitqp);
	return __SYSCALL_1(waitq_deinit,waitqp);
}
