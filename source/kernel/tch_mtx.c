/*
 * tch_mtx.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 *
 */



/*!\brief Mutex Implementation
 *  Mutex is simplest form of synchronization method in multi-threaded (or processed)
 *  Operating System. mutex can be implemented as system call so that mutex locking is
 *  performed in kernel mode. by
 *
 */

#include "tch_kernel.h"
#include "tch_mtx.h"

#define TCH_MTX_CLASS_KEY        ((uint16_t) 0x2D02)
#define MTX_VALIDATE(mcb)	do{\
	((tch_mtxCb*) mcb)->status = (((uint32_t) mcb ^ TCH_MTX_CLASS_KEY) & 0xFFFF);\
}while(0);

#define MTX_INVALIDATE(mcb)	do{\
	((tch_mtxCb*) mcb)->status &= ~0xFFFF;\
}while(0);

#define MTX_ISVALID(mcb)	((((tch_mtxCb*) mcb)->status & 0xFFFF) == (((uint32_t) mcb ^ TCH_MTX_CLASS_KEY) & 0xFFFF))


static tch_mtxId tch_mtx_create();
static tchStatus tch_mtx_lock(tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_mtx_unlock(tch_mtxId mtx);
static tchStatus tch_mtx_destroy(tch_mtxId mtx);


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
	if(!(--tch_currentThread->kthread->lckCnt)){
		tchk_threadSetPriority(mcb->own,mcb->svdPrior);
		mcb->svdPrior = Idle;
	}
	if(!tch_port_exclusiveCompareUpdate(&mcb->own,(uword_t) tch_currentThread,(uword_t)NULL))
		return tchErrorResource;
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

static tchStatus tch_mtx_destroy(tch_mtxId id){
	tchStatus result = tchOK;
	if(tch_port_isISR())
		return tchErrorISR;
	return (tchStatus) __SYSCALL_1(mutex_destroy,id);
}





