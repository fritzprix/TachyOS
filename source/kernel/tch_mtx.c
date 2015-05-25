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
#include "tch_mem.h"

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

void tch_mtxInit(tch_mtxCb* mcb){
	uStdLib->string->memset(mcb,0,sizeof(tch_mtxCb));
	tch_listInit((tch_lnode*)&mcb->que);
	mcb->own = NULL;
	mcb->__obj.__destr_fn = (tch_kobjDestr) __tch_noop_destr;
	mcb->status = 0;
	MTX_VALIDATE(mcb);
}

tchStatus tchk_mutex_lock(tch_mtxId mtx,uint32_t timeout){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	tch_threadId tid = (tch_threadId) tch_currentThread;           // get current thread id
	if(mcb->own == tid)
		return tchOK;                         // if this mutex is locked by current thread, return 'ok'
	else{
		if(!tch_port_exclusiveCompareUpdate((uaddr_t)&mcb->own,0,(uword_t)tid)){
			if(tch_currentThread)
			{
				tch_currentThread->t_kthread->t_lckCnt++;
				mcb->svdPrior = tch_currentThread->t_kthread->t_prior;
				return tchOK;
			}
		}else{
			tch_thread_prior prior = tchk_threadGetPriority(tid);
			if(tchk_threadGetPriority(mcb->own) < prior)
				tchk_threadSetPriority((tch_threadId)mcb->own,prior);
			tchk_schedThreadSuspend(&mcb->que,timeout);
		}
	}
	return tchOK;
}

tchStatus tchk_mutex_unlock(tch_mtxId mtx){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	if(!(--tch_currentThread->t_kthread->t_lckCnt)){
		tchk_threadSetPriority(mcb->own,mcb->svdPrior);
		mcb->svdPrior = Idle;
	}
	if(tch_port_exclusiveCompareUpdate(&mcb->own,(uword_t) tch_currentThread,(uword_t)NULL))
		return tchErrorResource;
	if(!tch_listIsEmpty(&mcb->que))
		tchk_schedThreadResumeM(&mcb->que,1,tchOK,TRUE);
	return tchOK;
}

tchStatus tchk_mutex_destroy(tch_mtxId mtx){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	MTX_INVALIDATE(mcb);
	tchk_threadSetPriority(tch_currentThread,mcb->svdPrior);
	mcb->svdPrior = Idle;
	tchk_shareableMemFree(mcb);
	tchk_schedThreadResumeM(&mcb->que,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	return tchOK;
}


static tch_mtxId tch_mtx_create(){
	tch_mtxCb* mcb = (tch_mtxCb*) tch_shMemAlloc(sizeof(tch_mtxCb),FALSE);
	uStdLib->string->memset(mcb,0,sizeof(tch_mtxCb));
	tch_listInit((tch_lnode*)&mcb->que);
	mcb->own = NULL;
	mcb->__obj.__destr_fn = (tch_kobjDestr) tch_mtx_destroy;
	mcb->status = 0;
	MTX_VALIDATE(mcb);
	return  mcb;
}


/***
 *  thread try lock mtx for given amount of time
 */
static tchStatus tch_mtx_lock(tch_mtxId id,uint32_t timeout){
	tchStatus result = tchOK;
	if(!id || !MTX_ISVALID(id))
		return tchErrorParameter;
	tch_mtxCb* mcb = (tch_mtxCb*) id;
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		do{
			result = tch_port_enterSv(SV_MTX_LOCK,(uword_t) id,timeout);
			if(!MTX_ISVALID(mcb))				// check mutex object after wakeup
				return result;
			if(mcb->own == tch_currentThread)		// check if mutex is successfully locked
				return result;						// if mutex locked by current thread,return with ok, otherwise retry or return with error
		}while(tch_currentThread->t_kRet == tchOK);
		return result;
	}
}


static tchStatus tch_mtx_unlock(tch_mtxId id){
	if(!id || !MTX_ISVALID(id)){
		return tchErrorResource;
	}
	tch_mtxCb* mcb = (tch_mtxCb*) id;
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	tch_threadId tid = Thread->self();
	return tch_port_enterSv(SV_MTX_UNLOCK,(uword_t) id,0);
}

static tchStatus tch_mtx_destroy(tch_mtxId id){
	tch_mtxCb* mcb = (tch_mtxCb*)id;
	tchStatus result = tchOK;
	if(tch_port_isISR())
		return tchErrorISR;
	if(!id || !MTX_ISVALID(mcb))
		return tchErrorResource;
	return tch_port_enterSv(SV_MTX_DESTROY,(uword_t)id,0);
}



