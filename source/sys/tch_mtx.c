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
#include "tch_sched.h"

#define MTX_ISVALID(mcb)      (((tch_mtxDef*)mcb)->key >= MTX_INIT_MARK) ||\
	                          ((!((tch_mtxDef*)mcb)->key) && (!((tch_mtxDef*)mcb)->own))

static tch_mtxId tch_mtx_create(tch_mtxDef* mcb);
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



tch_mtxId tch_mtx_create(tch_mtxDef* mcb){
	mcb->key = MTX_INIT_MARK;
	tch_listInit((tch_lnode_t*)&mcb->que);
	mcb->own = NULL;
	return  mcb;
}


/***
 *  thread try lock mtx for given amount of time
 */
#if !__FUTEX
tchStatus tch_mtx_lock(tch_mtxId mtx,uint32_t timeout){
	if(!mtx)
		return osErrorParameter;
 	tchStatus result = osOK;
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}
	if(!MTX_ISVALID(mtx))
		return osErrorResource;
	while(TRUE){
		result = tch_port_enterSvFromUsr(SV_MTX_LOCK,(uint32_t)mtx,timeout);
		switch(result){
		case osEventTimeout:
			return osErrorTimeoutResource;
		case osErrorResource:
			return osErrorResource;
		case osOK:
			if(((tch_mtxDef*)mtx)->key >= (uint32_t)tch_schedGetRunningThread()){
				return osOK;
			}
		}
	}
	return osErrorResource;

}
#else
tchStatus tch_mtx_lock(tch_mtxId id,uint32_t timeout){
	if(!id)                                   // check mtx id is not null
		return osErrorParameter;              // otherwise return 'osErrorParameter'
	tchStatus result = osOK;
	if(tch_port_isISR()){                     // if in isr return isr error
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}
	tch_mtxDef* mtx = (tch_mtxDef*) id;
	if(!MTX_ISVALID(mtx))                 // validity check of mutex control block
		return osErrorResource;
	int tid = (int)Thread->self();        // get current thread id
	if((int)mtx->own == tid)              // if this mutex is locked by current thread, return 'ok'
		return osOK;
	while(tch_port_exclusiveCompareUpdate((int*)&mtx->key,MTX_INIT_MARK,tid | MTX_INIT_MARK)){   // try exclusively lock
		tch_thread_prior prior = Thread->getPriorty((tch_threadId)tid);                         // mutex is already locked
		if(Thread->getPriorty(mtx->own) < prior)                                                 // compare current thread's priority
			Thread->setPriority(mtx->own,prior);                                                 // if more prior than current thread, priority of mutex owner will inherit current thread priority (* prevent priority inversion *)
		result = tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mtx->que,timeout);
		if(!MTX_ISVALID(mtx))                                                                    // validity of mutex is double-checked here, it's mutex can be destroyed after one of the wait thread is in ready state with system call result of 'osOk
			return osErrorResource;
		switch(result){
		case osEventTimeout:
			return osErrorTimeoutResource;
		case osErrorResource:
			return osErrorResource;
		}
	}
	if(result == osOK){
		mtx->own = (void*)tid;
		mtx->svdPrior = Thread->getPriorty((tch_threadId)tid);
		return osOK;
	}
	tch_kAssert(TRUE,osErrorOS);
	return osErrorOS;

}
#endif


#if !__FUTEX
tchStatus tch_mtx_unlock(tch_mtxId mtx){
	if(tch_port_isISR()){                               ///< check if in isr mode, then return osErrorISR
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}
	if(((tch_mtxDef*)mtx)->key < MTX_INIT_MARK){     ///< otherwise ensure this key is locked by any thread
		return osErrorResource;                    ///< if not, return osErrorParameter
	}
	return (tchStatus)tch_port_enterSvFromUsr(SV_MTX_UNLOCK,(uint32_t)mtx,0);   ///< otherwise, invoke system call for unlocking mtx

}
#else
tchStatus tch_mtx_unlock(tch_mtxId id){
	tch_mtxDef* mtx = (tch_mtxDef*) id;
	if(tch_port_isISR()){
		return osErrorISR;
	}
	tch_threadId tid = Thread->self();
	if(mtx->key != ((uint32_t)tid | MTX_INIT_MARK))
		return osErrorResource;
	if(!MTX_ISVALID(mtx))
		return osErrorResource;
	Thread->setPriority(mtx->own,mtx->svdPrior);
	mtx->svdPrior = Idle;
	mtx->own = NULL;
	mtx->key = MTX_INIT_MARK;
	if(!tch_listIsEmpty(&mtx->que))
		tch_port_enterSvFromUsr(SV_THREAD_RESUME,(uint32_t)&mtx->que,osOK);
	return osOK;
}
#endif

#if !__FUTEX
tchStatus tch_mtx_destroy(tch_mtxId mtx){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(!(((tch_mtxDef*)mtx)->key > MTX_INIT_MARK)){
			return osErrorResource;
		}
		((tch_mtxDef*)mtx)->key = 0;
		return (tchStatus)tch_port_enterSvFromUsr(SV_MTX_DESTROY,(uint32_t)mtx,0);
	}
}
#else
tchStatus tch_mtx_destroy(tch_mtxId id){
	tch_mtxDef* mtx = (tch_mtxDef*)id;
	if(!MTX_ISVALID(mtx))
		return osErrorResource;
	if(tch_mtx_lock(id,osWaitForever) != osOK)
		return osErrorTimeoutResource;
	mtx->key = 0;
	mtx->own = NULL;
	Thread->setPriority(Thread->self(),mtx->svdPrior);
	mtx->svdPrior = Idle;
	return (tchStatus)tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mtx->que,osErrorResource);
}
#endif
