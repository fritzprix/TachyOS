/*
 * tch_rendezvu.c
 *
 *  Created on: 2015. 12. 31.
 *      Author: doowoong lee
 */


#include "tch_kernel.h"
#include "tch_err.h"
#include "tch_kobj.h"
#include "tch_rendezvu.h"
#include "kernel/util/string.h"

#ifndef RENDEZVOUS_CLASS_KEY
#define RENDEZVOUS_CLASS_KEY			((uint16_t) 0xF53A)
#error "might not configured properly"
#endif


#define RENDEZV_VALIDATE(rndv)			do {\
	((tch_rendzvCb*) rndv)->status |= (((uint32_t) rndv ^ RENDEZVOUS_CLASS_KEY) & 0xFFFF);\
}while(0)

#define RENDEZV_INVALIDATE(rndv)			do {\
	((tch_rendzvCb*) rndv)->status &= ~0xFFFF;\
}while(0)

#define RENDEZV_ISVALID(rndv)			((((tch_rendzvCb*) rndv)->status & 0xFFFF) == (((uint32_t) rndv ^ RENDEZVOUS_CLASS_KEY) & 0xFFFF))

DECLARE_SYSCALL_0(rendv_create,tch_rendvId);
DECLARE_SYSCALL_2(rendv_sleep, tch_rendvId, uint32_t,tchStatus);
DECLARE_SYSCALL_1(rendv_wake, tch_rendvId, tchStatus);
DECLARE_SYSCALL_1(rendv_destroy, tch_rendvId,tchStatus);
DECLARE_SYSCALL_2(rendv_init,tch_rendzvCb*, BOOL, tchStatus);
DECLARE_SYSCALL_1(rendv_deinit,tch_rendzvCb*, tchStatus);

__USER_API__ static tch_rendvId tch_rendvCreate();
__USER_API__ static tchStatus tch_rendvSleep(tch_rendvId rendv,uint32_t timeout);
__USER_API__ static tchStatus tch_rendvWakeup(tch_rendvId rendv);
__USER_API__ static tchStatus tch_rendvDestroy(tch_rendvId rendv);

static tchStatus rendv_init(tch_rendzvCb* rndv,BOOL is_static);
static tchStatus rendv_deinit(tch_rendzvCb* rndv);

__USER_RODATA__ tch_rendezvu_api_t Rendezvous_IX = {
		.create = tch_rendvCreate,
		.sleep = tch_rendvSleep,
		.wake = tch_rendvWakeup,
		.destroy = tch_rendvDestroy
};

__USER_RODATA__ const tch_rendezvu_api_t* Rendezvous = &Rendezvous_IX;

DEFINE_SYSCALL_0(rendv_create,tch_rendvId)
{
	tch_rendzvCb* rndv = (tch_rendzvCb*) kmalloc(sizeof(tch_rendzvCb));
	if(!rndv)
		KERNEL_PANIC("rendezvous can't be created");
	if(rendv_init(rndv,FALSE) != tchOK)
	{
		kfree(rndv);
		return NULL;
	}
	return (tch_rendvId) rndv;
}

DEFINE_SYSCALL_2(rendv_sleep, tch_rendvId, rndv,uint32_t,timeout,tchStatus)
{
	if(!rndv)
		return tchErrorParameter;
	if(!RENDEZV_ISVALID(rndv))
		return tchErrorResource;
	tch_rendzvCb* rndvp = (tch_rendzvCb*) rndv;
	return tch_schedWait(&rndvp->wq, timeout);
}

DEFINE_SYSCALL_1(rendv_wake, tch_rendvId, rndv,tchStatus)
{
	if(!rndv)
		return tchErrorParameter;
	if(!RENDEZV_ISVALID(rndv))
		return tchErrorResource;
	tch_rendzvCb* rndvp = (tch_rendzvCb*) rndv;
	tch_schedWake(&rndvp->wq,SCHED_THREAD_ALL,tchOK,FALSE);
	return TRUE;
}

DEFINE_SYSCALL_1(rendv_destroy, tch_rendvId,rndv, tchStatus)
{
	if(!rndv || !RENDEZV_ISVALID(rndv))
		return tchErrorParameter;
	tchStatus res = rendv_deinit(rndv);
	kfree(rndv);
	return res;
}

DEFINE_SYSCALL_2(rendv_init, tch_rendzvCb*, rndv,BOOL, isstatic,tchStatus)
{
	if(!rndv)
		return tchErrorParameter;
	rendv_init(rndv,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(rendv_deinit, tch_rendzvCb*, rndv, tchStatus)
{
	if(!rndv || !RENDEZV_ISVALID(rndv))
		return tchErrorParameter;
	return rendv_deinit(rndv);
}


static tchStatus rendv_init(tch_rendzvCb* rndv,BOOL is_static)
{
	if(!rndv)
		return tchErrorParameter;
	mset(rndv, 0 ,sizeof(tch_rendzvCb));
	cdsl_dlistInit((cdsl_dlistNode_t*) &rndv->wq);
	tch_registerKobject(&rndv->__kobj,is_static? (tch_kobjDestr) tch_rendzvDeinit : (tch_kobjDestr) tch_rendvDestroy);
	RENDEZV_VALIDATE(rndv);
	return tchOK;
}

static tchStatus rendv_deinit(tch_rendzvCb* rndv)
{
	if(!rndv || !RENDEZV_ISVALID(rndv))
		return tchErrorParameter;
	RENDEZV_INVALIDATE(rndv);
	tch_unregisterKobject(&rndv->__kobj);
	tch_schedWake(&rndv->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	return tchOK;
}


__USER_API__ static tch_rendvId tch_rendvCreate()
{
	if(tch_port_isISR())
		return NULL;
	return (tch_rendvId) __SYSCALL_0(rendv_create);
}

__USER_API__ static tchStatus tch_rendvSleep(tch_rendvId rendv,uint32_t timeout)
{
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_2(rendv_sleep,rendv,timeout);
}

__USER_API__ static tchStatus tch_rendvWakeup(tch_rendvId rendv)
{
	if(tch_port_isISR())
		return __rendv_wake(rendv);
	return __SYSCALL_1(rendv_wake,rendv);
}

__USER_API__ static tchStatus tch_rendvDestroy(tch_rendvId rendv)
{
	if(!rendv)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(rendv_destroy,rendv);
}


tchStatus tch_rendzvInit(tch_rendzvCb* rendvp)
{
	if(!rendvp)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __rendv_init(rendvp, TRUE);
	return  __SYSCALL_2(rendv_init,rendvp, TRUE);
}

tchStatus tch_rendzvDeinit(tch_rendzvCb* rendvp)
{
	if(!rendvp)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __rendv_deinit(rendvp);
	return __SYSCALL_1(rendv_deinit,rendvp);
}
