/*
 * tch_lock.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 8. 9.
 *      Author: innocentevil
 */




#include "tch.h"
#include "kernel/tch_err.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_kobj.h"



/**
 *  macro for mutex
 */
#define TCH_MTX_CLASS_KEY        ((uint16_t) 0x2D02)
#define MTX_VALIDATE(mcb)	do{\
	((tch_mtxCb*) mcb)->status = (((uint32_t) mcb ^ TCH_MTX_CLASS_KEY) & 0xFFFF);\
}while(0);

#define MTX_INVALIDATE(mcb)	do{\
	((tch_mtxCb*) mcb)->status &= ~0xFFFF;\
}while(0);

#define MTX_ISVALID(mcb)	((((tch_mtxCb*) mcb)->status & 0xFFFF) == (((uint32_t) mcb ^ TCH_MTX_CLASS_KEY) & 0xFFFF))



/**
 * macro for condition variable
 */


#define CONDV_WAIT         			  ((uint32_t) 2 << 16)
#define TCH_CONDV_CLASS_KEY           ((uint16_t) 0x2D01)

#define tch_condvSetWait(condv)       ((tch_condvCb*) condv)->flags |= CONDV_WAIT
#define tch_condvClrWait(condv)       ((tch_condvCb*) condv)->flags &= ~CONDV_WAIT
#define tch_condvIsWait(condv)        (((tch_condvCb*) condv)->flags & CONDV_WAIT)


#define CONDV_VALIDATE(condv)	do{\
	((tch_condvCb*) condv)->flags = (((uint32_t) condv ^ TCH_CONDV_CLASS_KEY) & 0xFFFF);\
}while(0);

#define CONDV_INVALIDATE(condv)	do{\
	((tch_condvCb*) condv)->flags &= ~0xFFFF;\
}while(0);

#define CONDV_ISVALID(condv)	((((tch_condvCb*) condv)->flags & 0xFFFF) == (((uint32_t) condv ^ TCH_CONDV_CLASS_KEY) & 0xFFFF))



/**
 * Declaration for mutex api
 */

static tch_mtxId tch_mutexCreate();
static tchStatus tch_mutexLock(tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_mutexUnlock(tch_mtxId mtx);
static tchStatus tch_mutexDestroy(tch_mtxId mtx);
static tchStatus tch_mutexUnlockFromCondv(tch_mtxId mtx);
static tch_mtxId mutex_init(tch_mtxCb* mcb,BOOL is_static);
static tchStatus mutex_deinit(tch_mtxCb* mcb);




__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mutexCreate,
		tch_mutexLock,
		tch_mutexUnlock,
		tch_mutexDestroy,
};

const tch_mtx_ix* Mtx = &MTX_Instance;

DECLARE_SYSCALL_0(mutex_create,tch_mtxId);
DECLARE_SYSCALL_2(mutex_lock,tch_mtxId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(mutex_unlock,tch_mtxId,tchStatus);
DECLARE_SYSCALL_1(mutex_destroy,tch_mtxId,tchStatus);
DECLARE_SYSCALL_1(mutex_init,tch_mtxCb*,tchStatus);
DECLARE_SYSCALL_1(mutex_deinit,tch_mtxCb*,tchStatus);


/**
 * Declaration for condv api
 */

static tch_condvId tch_condvCreate();
static tchStatus tch_condvWait(tch_condvId condv,tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_condvWake(tch_condvId condv);
static tchStatus tch_condvWakeAll(tch_condvId condv);
static tchStatus tch_condvDestroy(tch_condvId condv);

static tch_condvId condv_init(tch_condvCb* condv,BOOL is_static);
static tchStatus condv_deint(tch_condvCb* condv);

struct condv_param {
	tch_condvId id;
	void*		arg;
};

__attribute__((section(".data"))) static tch_condv_ix CondVar_StaticInstance = {
		.create = tch_condvCreate,
		.wait = tch_condvWait,
		.wake = tch_condvWake,
		.wakeAll = tch_condvWakeAll,
		.destroy = tch_condvDestroy
};


const tch_condv_ix* Condv = &CondVar_StaticInstance;



DECLARE_SYSCALL_0(condv_create,tch_condvId);
DECLARE_SYSCALL_3(condv_wait,tch_condvId,tch_mtxId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(condv_wake,struct condv_param*,tchStatus);
DECLARE_SYSCALL_1(condv_wakeAll,struct condv_param*, tchStatus);
DECLARE_SYSCALL_1(condv_destroy,tch_condvId,tchStatus);
DECLARE_SYSCALL_1(condv_init,tch_condvCb*,tchStatus);
DECLARE_SYSCALL_1(condv_deinit,tch_condvCb*,tchStatus);

/**
 *
 */

DEFINE_SYSCALL_0(mutex_create,tch_mtxId){
	tch_mtxCb* mcb = (tch_mtxCb*) kmalloc(sizeof(tch_mtxCb));
	mutex_init(mcb,FALSE);
	return (tch_mtxId) mcb;
}


DEFINE_SYSCALL_2(mutex_lock,tch_mtxId, mtx,uint32_t, timeout,tchStatus){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	tch_threadId tid = (tch_threadId) current;           // get current thread id
	if(!MTX_ISVALID(mcb))				// check mutex object after wakeup
		return tchErrorResource;
	if(mcb->own == tid)
		return tchOK;                         // if this mutex is locked by current thread, return 'ok'
	else{
		if(tch_port_exclusiveCompareUpdate((uaddr_t)&mcb->own,0,(uword_t)tid)){
			if(current)
			{
				current->kthread->lckCnt++;
				mcb->svdPrior = current->kthread->prior;
				return (tchStatus) tchOK;
			}
		}else{
			tch_threadPrior prior = tch_threadGetPriority(tid);
			if(tch_threadGetPriority(mcb->own) < prior)
				tch_threadSetPriority((tch_threadId)mcb->own,prior);
			tch_schedWait(&mcb->que,timeout);
		}
	}
	return tchOK;
}


DEFINE_SYSCALL_1(mutex_unlock,tch_mtxId, mtx, tchStatus){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	if(!tch_port_exclusiveCompareUpdate(&mcb->own,(uword_t) current,(uword_t)NULL))
		return tchErrorResource;
	if(!(--current->kthread->lckCnt)){
		tch_threadSetPriority(mcb->own,mcb->svdPrior);
		mcb->svdPrior = Idle;
	}

	if(!cdsl_dlistIsEmpty(&mcb->que))
		tch_schedWake(&mcb->que,1,tchInterrupted,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(mutex_destroy,tch_mtxId, mtx , tchStatus){
	tchStatus result = mutex_deinit(mtx);
	if(result != tchOK)
		return result;

	kfree(mtx);
	return result;
}

DEFINE_SYSCALL_1(mutex_init,tch_mtxCb*,mp,tchStatus){
	if(!mp)
		return tchErrorParameter;
	mutex_init(mp,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(mutex_deinit,tch_mtxCb*,mp,tchStatus){
	if(!mp || !MTX_ISVALID(mp))
		return tchErrorParameter;
	return mutex_deinit(mp);
}

static tch_mtxId mutex_init(tch_mtxCb* mcb,BOOL is_static){
	mcb->svdPrior = Normal;
	cdsl_dlistInit((cdsl_dlistNode_t*)&mcb->que);
	mcb->own = NULL;
	tch_registerKobject(&mcb->__obj,is_static? (tch_kobjDestr) mutex_deinit :  (tch_kobjDestr) tch_mutexDestroy);
	mcb->status = 0;
	MTX_VALIDATE(mcb);
	return mcb;
}

static tchStatus mutex_deinit(tch_mtxCb* mcb){
	if(!MTX_ISVALID(mcb))
		return tchErrorParameter;
	MTX_INVALIDATE(mcb);
	tch_threadSetPriority(current,mcb->svdPrior);
	mcb->svdPrior = Idle;
	tch_schedWake(&mcb->que,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tch_unregisterKobject(&mcb->__obj);
	return tchOK;
}

static tch_mtxId tch_mutexCreate(){
	if(tch_port_isISR())
		return NULL;
	return (tch_mtxId) __SYSCALL_0(mutex_create);
}

static tchStatus tch_mutexLock(tch_mtxId id,uint32_t timeout){
	tchStatus result = tchOK;
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	do {
		result = (tchStatus) __SYSCALL_2(mutex_lock,id,timeout);
		if (result == tchOK) // check if mutex is successfully locked
			return result; // if mutex locked by current thread,return with ok, otherwise retry or return with error
	} while (result == tchInterrupted);
	return result;
}

static tchStatus tch_mutexUnlock(tch_mtxId id){
	if(!id || !MTX_ISVALID(id))
		return tchErrorResource;
	if(tch_port_isISR())
		return tchErrorISR;
	return (tchStatus) __SYSCALL_1(mutex_unlock,id);
}

static tchStatus tch_mutexUnlockFromCondv(tch_mtxId mtx){
	if(!mtx || !MTX_ISVALID(mtx))
		return tchErrorParameter;

	tch_mtxCb* lock = (tch_mtxCb*) mtx;
	if(!tch_port_exclusiveCompareUpdate(&lock->own,(uword_t) current,(uword_t)NULL))
		return tchErrorParameter;			// current thread is not owner of mutex lock

	if(!(--current->kthread->lckCnt)){
		tch_threadSetPriority(lock->own,lock->svdPrior);
		lock->svdPrior = Idle;
	}
	if(!cdsl_dlistIsEmpty(&lock->que))		// if valid owner , move waiting thread from mutex wait queue to ready queue not preemptive way
		tch_schedWake(&lock->que,1,tchInterrupted,FALSE);
	return tchOK;
}


static tchStatus tch_mutexDestroy(tch_mtxId id){
	tchStatus result = tchOK;
	if(tch_port_isISR())
		return tchErrorISR;
	return (tchStatus) __SYSCALL_1(mutex_destroy,id);
}


tchStatus tch_mutexInit(tch_mtxCb* mcb){
	if(!mcb)
		return tchErrorParameter;
	if(tch_port_isISR() || !kernel_ready)
		return __mutex_init(mcb);
	return __SYSCALL_1(mutex_init,mcb);
}

tchStatus tch_mutexDeinit(tch_mtxCb* mcb){
	if(!mcb || !MTX_ISVALID(mcb))
		return tchErrorParameter;
	if(tch_port_isISR())
		return __mutex_deinit(mcb);
	return __SYSCALL_1(mutex_deinit,mcb);
}


DEFINE_SYSCALL_0(condv_create,tch_condvId){
	tch_condvCb* condv = (tch_condvCb*) kmalloc(sizeof(tch_condvCb));
	if(!condv)
		KERNEL_PANIC("tch_condv.c","can't create condition variable object");
	condv_init(condv,FALSE);
	return (tch_condvId) condv;
}

DEFINE_SYSCALL_3(condv_wait,tch_condvId, condvId, tch_mtxId, mtxId,uint32_t,timeout,tchStatus){
	if(!condvId || !CONDV_ISVALID(condvId) || !MTX_ISVALID(mtxId))
		return tchErrorParameter;

	tch_condvCb* condv = (tch_condvCb*) condvId;
	tch_mtxCb* lock = (tch_mtxCb*) mtxId;
	tchStatus result = tchOK;
	condv->waitMtx = mtxId;

	/**
	 * unlock mutexlock
	 */
	if((result = tch_mutexUnlockFromCondv(condv->waitMtx)) != tchOK)
		return result;
	/**
	 * block current thread into wait in the thread queue of condition variable
	 */
	tch_schedWait(&condv->wq,timeout);
	tch_condvSetWait(condv);

	return tchOK;			// kernel result of current thread
}

DEFINE_SYSCALL_1(condv_wake,struct condv_param*,cparm,tchStatus){
	if(!cparm)
		return tchErrorParameter;
	tch_condvCb* condv = (tch_condvCb*) cparm->id;

	if(!condv || !CONDV_ISVALID(condv) || !tch_condvIsWait(condv))
		return tchErrorParameter;

	tchStatus result = tchOK;
	if((result = tch_mutexUnlockFromCondv(condv->waitMtx)) != tchOK)
		return result;
	tch_condvClrWait(condv);
	tch_schedWake(&condv->wq,1,tchInterrupted,TRUE);			// wake thread from condv block queue with allowance of preemption
	cparm->arg = condv->waitMtx;

	return tchOK;
}

DEFINE_SYSCALL_1(condv_wakeAll,struct condv_param*,cparm,tchStatus) {
	if(!cparm)
		return tchErrorParameter;
	tch_condvCb* condv = (tch_condvCb*) cparm->id;

	if(!condv || !CONDV_ISVALID(condv) || !tch_condvIsWait(condv))
		return tchErrorParameter;

	tchStatus result = tchOK;
	if((result = tch_mutexUnlockFromCondv(condv->waitMtx)) != tchOK)
		return result;
	tch_condvClrWait(condv);
	tch_schedWake(&condv->wq,SCHED_THREAD_ALL,tchInterrupted,TRUE);			// wake thread from condv block queue with allowance of preemption
	cparm->arg = condv->waitMtx;

	return tchOK;
}

DEFINE_SYSCALL_1(condv_destroy,tch_condvId,id,tchStatus){
	tchStatus result = condv_deint(id);
	kfree(id);
	return result;
}


DEFINE_SYSCALL_1(condv_init,tch_condvCb*,cp,tchStatus){
	if(!cp)
		return tchErrorParameter;
	condv_init(cp,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(condv_deinit,tch_condvCb*,cp,tchStatus){
	if(!cp || !CONDV_ISVALID(cp))
		return tchErrorParameter;
	condv_deint(cp);
	return tchOK;
}


static tch_condvId condv_init(tch_condvCb* condv,BOOL is_static){
	memset(condv,0,sizeof(tch_condvCb));
	cdsl_dlistInit((cdsl_dlistNode_t*)&condv->wq);
	condv->waitMtx = NULL;
	CONDV_VALIDATE(condv);
	tch_registerKobject(&condv->__obj,is_static? (tch_kobjDestr)condv_deint : (tch_kobjDestr)tch_condvDestroy);
	return (tch_condvId) condv;
}

static tchStatus condv_deint(tch_condvCb* condv){
	if(!condv)
		return tchErrorParameter;
	if(!CONDV_ISVALID(condv))
		return tchErrorResource;
	tch_unregisterKobject(&condv->__obj);
	return tch_schedWake(&condv->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
}

static tch_condvId tch_condvCreate(){
	if(tch_port_isISR())
		return NULL;
	return (tch_condvId)__SYSCALL_0(condv_create);
}


/*! \brief thread wait until given condition is met
 *  \
 *
 */
static tchStatus tch_condvWait(tch_condvId id,tch_mtxId lock,uint32_t timeout){
	if(!id || !lock)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	tchStatus result = tchOK;
	if((result = __SYSCALL_3(condv_wait,id,lock,timeout)) != tchInterrupted)
		return result;

	return tch_mutexLock(lock,timeout);			// try acquire lock again
}

/*!
 * \brief posix condition variable signal
 */
static tchStatus tch_condvWake(tch_condvId id){
	tch_condvCb* condv = (tch_condvCb*) id;
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())                  // if isr mode, no locked mtx is supplied
		return tchErrorISR;

	struct condv_param cparm;
	cparm.id = id;
	cparm.arg = NULL;

	//returned cparm has value of mutex id relevant to this condition variable
	tchStatus result = __SYSCALL_1(condv_wake,&cparm);
	tch_mutexLock((tch_mtxId) cparm.arg,tchWaitForever);
	return result;
}

static tchStatus tch_condvWakeAll(tch_condvId id){
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;

	struct condv_param cparm;
	cparm.id = id;
	cparm.arg = NULL;

	tchStatus result = __SYSCALL_1(condv_wakeAll,&cparm);
	tch_mutexLock((tch_mtxId) cparm.arg,tchWaitForever);
	return result;
}

static tchStatus tch_condvDestroy(tch_condvId id){
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(condv_destroy,id);
}


tchStatus tch_condvInit(tch_condvCb* condv){
	if(!condv)
		return tchErrorParameter;
	if(tch_port_isISR() || !kernel_ready)
		return __condv_init(condv);
	return __SYSCALL_1(condv_init,condv);
}

tchStatus tch_condvDeint(tch_condvCb* condv){
	if(!condv)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __condv_deinit(condv);
	return __SYSCALL_1(condv_deinit,condv);
}

