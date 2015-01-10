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

static tch_mtxId tch_mtx_create();
static tchStatus tch_mtx_lock(tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_mtx_unlock(tch_mtxId mtx);
static tchStatus tch_mtx_destroy(tch_mtxId mtx);





#define TCH_MTX_CLASS_KEY        ((uint16_t) 0x2D02)


static inline void tch_mtxValidate(tch_mtxId mtx);
static inline void tch_mtxInvalidate(tch_mtxId mtx);
static inline BOOL tch_mtxIsValid(tch_mtxId mtx);






__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mtx_create,
		tch_mtx_lock,
		tch_mtx_unlock,
		tch_mtx_destroy,
};

const tch_mtx_ix* Mtx = &MTX_Instance;



void tch_mtxInit(tch_mtxCb* mcb){
	uStdLib->string->memset(mcb,0,sizeof(tch_mtxCb));
	tch_listInit((tch_lnode_t*)&mcb->que);
	mcb->own = NULL;
	mcb->__obj.destructor = (tch_uobjDestr) tch_noop_destr;
	mcb->state = 0;
	tch_mtxValidate(mcb);
}

tch_mtxId tch_mtx_create(){
	tch_mtxCb* mcb = (tch_mtxCb*) shMem->alloc(sizeof(tch_mtxCb));
	uStdLib->string->memset(mcb,0,sizeof(tch_mtxCb));
	tch_listInit((tch_lnode_t*)&mcb->que);
	mcb->own = NULL;
	mcb->__obj.destructor = (tch_uobjDestr) tch_mtx_destroy;
	mcb->state = 0;
	tch_mtxValidate(mcb);
	return  mcb;
}


/***
 *  thread try lock mtx for given amount of time
 */
tchStatus tch_mtx_lock(tch_mtxId id,uint32_t timeout){
	if(!id)                                   // check mtx id is not null
		return tchErrorParameter;              // otherwise return 'tchErrorParameter'
	tchStatus result = tchOK;
	if(tch_port_isISR()){                     // if in isr return isr error
		tch_kernel_errorHandler(FALSE,tchErrorISR);
		return tchErrorISR;
	}
	tch_mtxCb* mcb = (tch_mtxCb*) id;
	if(!tch_mtxIsValid(mcb))
		return tchErrorResource;              // validity check of mutex control block
	tch_threadId tid = Thread->self();           // get current thread id
	if(mcb->own == tid)
		return tchOK;                         // if this mutex is locked by current thread, return 'ok'
	while(tch_port_exclusiveCompareUpdate((uaddr_t)&mcb->own,0,(uword_t) tid)){
		tch_thread_prior prior = Thread->getPriorty((tch_threadId)tid);                         // mutex is already locked
		if(Thread->getPriorty((tch_threadId)mcb->own) < prior)
			Thread->setPriority((tch_threadId)mcb->own,prior);
		result = tch_port_enterSv(SV_THREAD_SUSPEND,(uint32_t)&mcb->que,timeout);
		if(!tch_mtxIsValid(mcb))
			return tchErrorResource;
		switch(result){
		case tchEventTimeout:
			return tchErrorTimeoutResource;
		case tchErrorResource:
			return tchErrorResource;
		}
	}
	if(result == tchOK){
		mcb->svdPrior = Thread->getPriorty((tch_threadId) tid);
		mcb->own = (tch_threadId) tid;
		return tchOK;
	}
	return result;
}


tchStatus tch_mtx_unlock(tch_mtxId id){
	tch_mtxCb* mcb = (tch_mtxCb*) id;
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	tch_threadId tid = Thread->self();
	if(mcb->own != tid)
		return tchErrorResource;
	if(!tch_mtxIsValid(mcb))
		return tchErrorResource;
	Thread->setPriority(mcb->own,mcb->svdPrior);
	mcb->svdPrior = Idle;
	mcb->own = NULL;
	if(!tch_listIsEmpty(&mcb->que))
		tch_port_enterSv(SV_THREAD_RESUME,(uint32_t)&mcb->que,tchOK);
	return tchOK;
}

tchStatus tch_mtx_destroy(tch_mtxId id){
	tch_mtxCb* mcb = (tch_mtxCb*)id;
	tchStatus result = tchOK;
	if(tch_port_isISR())
		return tchErrorISR;
	if(!tch_mtxIsValid(mcb))
		return tchErrorResource;
	if(tch_mtx_lock(id,tchWaitForever) != tchOK)
		return tchErrorTimeoutResource;
	tch_mtxInvalidate(mcb);
	result = tch_port_enterSv(SV_THREAD_RESUMEALL,(uint32_t)&mcb->que,tchErrorResource);
	mcb->own = NULL;
	Thread->setPriority(Thread->self(),mcb->svdPrior);
	mcb->svdPrior = Idle;
	shMem->free(mcb);
	return result;
}


static inline void tch_mtxValidate(tch_mtxId mtx){
	tch_mtxCb* mcb = (tch_mtxCb*) mtx;
	mcb->state = (TCH_MTX_CLASS_KEY ^ ((uint32_t)mtx & 0xFFFF)) << 2;
}


static inline void tch_mtxInvalidate(tch_mtxId mtx){
	((tch_mtxCb*) mtx)->state &= ~(0xFFFF << 2);
}
static inline BOOL tch_mtxIsValid(tch_mtxId mtx){
	return (((((tch_mtxCb*) mtx)->state >> 2) & 0xFFFF) ^ TCH_MTX_CLASS_KEY) == ((uint32_t)mtx & 0xFFFF);
}

