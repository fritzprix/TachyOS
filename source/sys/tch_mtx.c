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

typedef struct tch_mtx_cb {
	tch_uobj            __obj;
	uint32_t            state;
	tch_thread_queue    que;
	tch_threadId        own;
	tch_thread_prior    svdPrior;
}tch_mtx;



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



tch_mtxId tch_mtx_create(){
	tch_mtx* mcb = (tch_mtx*) shMem->alloc(sizeof(tch_mtx));
	uStdLib->string->memset(mcb,0,sizeof(tch_mtx));
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
		return osErrorParameter;              // otherwise return 'osErrorParameter'
	tchStatus result = osOK;
	if(tch_port_isISR()){                     // if in isr return isr error
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}
	tch_mtx* mcb = (tch_mtx*) id;
	if(!tch_mtxIsValid(mcb))
		return osErrorResource;              // validity check of mutex control block
	tch_threadId tid = Thread->self();           // get current thread id
	if(mcb->own == tid)
		return osOK;                         // if this mutex is locked by current thread, return 'ok'
	while(tch_port_exclusiveCompareUpdate((uaddr_t)&mcb->own,0,(uword_t) tid)){
		tch_thread_prior prior = Thread->getPriorty((tch_threadId)tid);                         // mutex is already locked
		if(Thread->getPriorty((tch_threadId)mcb->own) < prior)
			Thread->setPriority((tch_threadId)mcb->own,prior);
		result = tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mcb->que,timeout);
		if(!tch_mtxIsValid(mcb))
			return osErrorResource;
		switch(result){
		case osEventTimeout:
			return osErrorTimeoutResource;
		case osErrorResource:
			return osErrorResource;
		}
	}
	if(result == osOK){
		mcb->svdPrior = Thread->getPriorty((tch_threadId) tid);
		mcb->own = (tch_threadId) tid;
		return osOK;
	}
	return result;
}


tchStatus tch_mtx_unlock(tch_mtxId id){
	tch_mtx* mcb = (tch_mtx*) id;
	if(tch_port_isISR()){
		return osErrorISR;
	}
	tch_threadId tid = Thread->self();
	if(mcb->own != tid)
		return osErrorResource;
	if(!tch_mtxIsValid(mcb))
		return osErrorResource;
	Thread->setPriority(mcb->own,mcb->svdPrior);
	mcb->svdPrior = Idle;
	mcb->own = NULL;
	if(!tch_listIsEmpty(&mcb->que))
		tch_port_enterSvFromUsr(SV_THREAD_RESUME,(uint32_t)&mcb->que,osOK);
	return osOK;
}

tchStatus tch_mtx_destroy(tch_mtxId id){
	tch_mtx* mcb = (tch_mtx*)id;
	tchStatus result = osOK;
	if(tch_port_isISR())
		return osErrorISR;
	if(!tch_mtxIsValid(mcb))
		return osErrorResource;
	if(tch_mtx_lock(id,osWaitForever) != osOK)
		return osErrorTimeoutResource;
	tch_mtxInvalidate(mcb);
	result = tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mcb->que,osErrorResource);
	mcb->own = NULL;
	Thread->setPriority(Thread->self(),mcb->svdPrior);
	mcb->svdPrior = Idle;
	shMem->free(mcb);
	return osOK;
}


static inline void tch_mtxValidate(tch_mtxId mtx){
	tch_mtx* mcb = (tch_mtx*) mtx;
	mcb->state = (TCH_MTX_CLASS_KEY ^ ((uint32_t)mtx & 0xFFFF)) << 2;
}


static inline void tch_mtxInvalidate(tch_mtxId mtx){
	((tch_mtx*) mtx)->state &= ~(0xFFFF << 2);
}
static inline BOOL tch_mtxIsValid(tch_mtxId mtx){
	return (((((tch_mtx*) mtx)->state >> 2) & 0xFFFF) ^ TCH_MTX_CLASS_KEY) == ((uint32_t)mtx & 0xFFFF);
}

