/*
 * tch_lock.c
 *
 *  Created on: 2015. 8. 9.
 *      Author: innocentevil
 */




#include "tch.h"
#include "kernel/tch_err.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"



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

static tch_mtxId tch_mtx_create();
static tchStatus tch_mtx_lock(tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_mtx_unlock(tch_mtxId mtx);
static tchStatus tch_mtx_destroy(tch_mtxId mtx);
static tchStatus tchk_mtx_unlock_from_condv(tch_mtxId mtx);


__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mtx_create,
		tch_mtx_lock,
		tch_mtx_unlock,
		tch_mtx_destroy,
};

const tch_mtx_ix* Mtx = &MTX_Instance;

DECLARE_SYSCALL_0(mutex_create,tch_mtxId);
DECLARE_SYSCALL_2(mutex_lock,tch_mtxId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(mutex_unlock,tch_mtxId,tchStatus);
DECLARE_SYSCALL_1(mutex_destroy,tch_mtxId,tchStatus);


/**
 * Declaration for condv api
 */

static tch_condvId tch_condv_create();
static tchStatus tch_condv_wait(tch_condvId condv,tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_condv_wake(tch_condvId condv);
static tchStatus tch_condv_wakeAll(tch_condvId condv);
static tchStatus tch_condv_destroy(tch_condvId condv);

struct condv_param {
	tch_condvId id;
	void*		arg;
};

__attribute__((section(".data"))) static tch_condv_ix CondVar_StaticInstance = {
		.create = tch_condv_create,
		.wait = tch_condv_wait,
		.wake = tch_condv_wake,
		.wakeAll = tch_condv_wakeAll,
		.destroy = tch_condv_destroy
};


const tch_condv_ix* Condv = &CondVar_StaticInstance;



DECLARE_SYSCALL_0(condv_create,tch_condvId);
DECLARE_SYSCALL_3(condv_wait,tch_condvId,tch_mtxId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(condv_wake,struct condv_param*,tchStatus);
DECLARE_SYSCALL_1(condv_wakeAll,struct condv_param*, tchStatus);
DECLARE_SYSCALL_1(condv_destroy,tch_condvId,tchStatus);

/**
 *
 */

DEFINE_SYSCALL_0(mutex_create,tch_mtxId){
	tch_mtxCb* mcb = (tch_mtxCb*) kmalloc(sizeof(tch_mtxCb));
	tchk_mutexInit(mcb,FALSE);
	return (tch_mtxId) mcb;
}


DEFINE_SYSCALL_2(mutex_lock,tch_mtxId, mtx,uint32_t, timeout,tchStatus){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	tch_threadId tid = (tch_threadId) tch_currentThread;           // get current thread id
	if(!MTX_ISVALID(mcb))				// check mutex object after wakeup
		return tchErrorResource;
	if(mcb->own == tid)
		return tchOK;                         // if this mutex is locked by current thread, return 'ok'
	else{
		if(tch_port_exclusiveCompareUpdate((uaddr_t)&mcb->own,0,(uword_t)tid)){
			if(tch_currentThread)
			{
				tch_currentThread->kthread->lckCnt++;
				mcb->svdPrior = tch_currentThread->kthread->prior;
				return (tchStatus) tchOK;
			}
		}else{
			tch_threadPrior prior = tchk_threadGetPriority(tid);
			if(tchk_threadGetPriority(mcb->own) < prior)
				tchk_threadSetPriority((tch_threadId)mcb->own,prior);
			tchk_schedThreadSuspend(&mcb->que,timeout);
		}
	}
	return tchOK;
}


DEFINE_SYSCALL_1(mutex_unlock,tch_mtxId, mtx, tchStatus){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	if(!tch_port_exclusiveCompareUpdate(&mcb->own,(uword_t) tch_currentThread,(uword_t)NULL))
		return tchErrorResource;
	if(!(--tch_currentThread->kthread->lckCnt)){
		tchk_threadSetPriority(mcb->own,mcb->svdPrior);
		mcb->svdPrior = Idle;
	}

	if(!cdsl_dlistIsEmpty(&mcb->que))
		tchk_schedThreadResumeM(&mcb->que,1,tchInterrupted,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(mutex_destroy,tch_mtxId, mtx , tchStatus){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	if(!MTX_ISVALID(mcb))
		return tchErrorParameter;
	MTX_INVALIDATE(mcb);
	tchk_threadSetPriority(tch_currentThread,mcb->svdPrior);
	mcb->svdPrior = Idle;
	tchk_shmFree(mcb);
	tchk_schedThreadResumeM(&mcb->que,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	return tchOK;
}


tch_mtxId tchk_mutexInit(tch_mtxCb* mcb,BOOL is_static){
	mcb->svdPrior = Normal;
	cdsl_dlistInit((cdsl_dlistNode_t*)&mcb->que);
	mcb->own = NULL;
	mcb->__obj.__destr_fn = is_static? (tch_kobjDestr) __tch_noop_destr :  (tch_kobjDestr) tch_mtx_destroy;
	mcb->status = 0;
	MTX_VALIDATE(mcb);
	return mcb;
}


static tch_mtxId tch_mtx_create(){
	if(tch_port_isISR())
		return NULL;
	return (tch_mtxId) __SYSCALL_0(mutex_create);
}

static tchStatus tch_mtx_lock(tch_mtxId id,uint32_t timeout){
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

static tchStatus tch_mtx_unlock(tch_mtxId id){
	if(!id || !MTX_ISVALID(id))
		return tchErrorResource;
	if(tch_port_isISR())
		return tchErrorISR;
	return (tchStatus) __SYSCALL_1(mutex_unlock,id);
}

static tchStatus tchk_mtx_unlock_from_condv(tch_mtxId mtx){
	if(!mtx || !MTX_ISVALID(mtx))
		return tchErrorParameter;

	tch_mtxCb* lock = (tch_mtxCb*) mtx;
	if(!tch_port_exclusiveCompareUpdate(&lock->own,(uword_t) tch_currentThread,(uword_t)NULL))
		return tchErrorParameter;			// current thread is not owner of mutex lock

	if(!(--tch_currentThread->kthread->lckCnt)){
		tchk_threadSetPriority(lock->own,lock->svdPrior);
		lock->svdPrior = Idle;
	}
	if(!cdsl_dlistIsEmpty(&lock->que))		// if valid owner , move waiting thread from mutex wait queue to ready queue not preemptive way
		tchk_schedThreadResumeM(&lock->que,1,tchInterrupted,FALSE);
	return tchOK;
}


static tchStatus tch_mtx_destroy(tch_mtxId id){
	tchStatus result = tchOK;
	if(tch_port_isISR())
		return tchErrorISR;
	return (tchStatus) __SYSCALL_1(mutex_destroy,id);
}

DEFINE_SYSCALL_0(condv_create,tch_condvId){
	tch_condvCb* condv = (tch_condvCb*) kmalloc(sizeof(tch_condvCb));
	if(!condv)
		KERNEL_PANIC("tch_condv.c","can't create condition variable object");
	tchk_condvInit(condv,FALSE);
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
	if((result = tchk_mtx_unlock_from_condv(condv->waitMtx)) != tchOK)
		return result;
	/**
	 * block current thread into wait in the thread queue of condition variable
	 */
	tchk_schedThreadSuspend(&condv->wq,timeout);
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
	if((result = tchk_mtx_unlock_from_condv(condv->waitMtx)) != tchOK)
		return result;
	tch_condvClrWait(condv);
	tchk_schedThreadResumeM(&condv->wq,1,tchInterrupted,TRUE);			// wake thread from condv block queue with allowance of preemption
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
	if((result = tchk_mtx_unlock_from_condv(condv->waitMtx)) != tchOK)
		return result;
	tch_condvClrWait(condv);
	tchk_schedThreadResumeM(&condv->wq,SCHED_THREAD_ALL,tchInterrupted,TRUE);			// wake thread from condv block queue with allowance of preemption
	cparm->arg = condv->waitMtx;

	return tchOK;
}


/**
 * 	tch_condvCb* condv = (tch_condvCb*) id;
	tchStatus result = tchOK;
	if(!tch_condvIsValid(condv))
		return tchErrorResource;
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		Mtx->lock(condv->waitMtx,tchWaitForever);
		tch_condvInvalidate(condv);
		result = tch_port_enterSv(SV_THREAD_RESUMEALL,(uword_t)&condv->wq,tchErrorResource,0);
		Mtx->unlock(condv->waitMtx);
		kfree(condv);
		return result;
	}
 */
DEFINE_SYSCALL_1(condv_destroy,tch_condvId,id,tchStatus){
	tchStatus result = tchOK;
	if(!id)
		return tchErrorParameter;
	if(!CONDV_ISVALID(id))
		return tchErrorResource;
	tch_condvCb* condv = (tch_condvCb*) id;
	kfree(condv);
	return tchk_schedThreadResumeM(&condv->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
}

tch_condvId tchk_condvInit(tch_condvCb* condv,BOOL is_static){
	memset(condv,0,sizeof(tch_condvCb));
	cdsl_dlistInit((cdsl_dlistNode_t*)&condv->wq);
	condv->waitMtx = NULL;
	CONDV_VALIDATE(condv);
	condv->__obj.__destr_fn =  is_static? (tch_kobjDestr)__tch_noop_destr : (tch_kobjDestr)tch_condv_destroy;
	return (tch_condvId) condv;
}


static tch_condvId tch_condv_create(){
	if(tch_port_isISR())
		return NULL;
	return (tch_condvId)__SYSCALL_0(condv_create);
}


/*! \brief thread wait until given condition is met
 *  \
 *
 */
static tchStatus tch_condv_wait(tch_condvId id,tch_mtxId lock,uint32_t timeout){
	if(!id || !lock)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	tchStatus result = __SYSCALL_3(condv_wait,id,lock,timeout);
	if(result != tchInterrupted)
		return result;

	return tch_mtx_lock(lock,timeout);			// try acquire lock again
}

/*!
 * \brief posix condition variable signal
 */
static tchStatus tch_condv_wake(tch_condvId id){
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
	if(result != tchOK)
		return result;

	return tch_mtx_lock((tch_mtxId) cparm.arg,tchWaitForever);
}

static tchStatus tch_condv_wakeAll(tch_condvId id){
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;

	struct condv_param cparm;
	cparm.id = id;
	cparm.arg = NULL;

	tchStatus result = __SYSCALL_1(condv_wakeAll,&cparm);
	if(result != tchOK)
		return result;
	return tch_mtx_lock((tch_mtxId) cparm.arg,tchWaitForever);
}

static tchStatus tch_condv_destroy(tch_condvId id){
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(condv_destroy,id);
}


