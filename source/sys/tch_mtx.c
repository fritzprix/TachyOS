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
 */



#include "tch_kernel.h"
#include "tch_sched.h"



static tch_mtx_id tch_mtx_create(tch_mtxDef* mcb);
static tchStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout);
static tchStatus tch_mtx_unlock(tch_mtx_id mtx);
static tchStatus tch_mtx_destroy(tch_mtx_id mtx);



__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mtx_create,
		tch_mtx_lock,
		tch_mtx_unlock,
		tch_mtx_destroy,
};

const tch_mtx_ix* Mtx = &MTX_Instance;



tch_mtx_id tch_mtx_create(tch_mtxDef* mcb){
	mcb->key = MTX_INIT_MARK;
	tch_listInit((tch_lnode_t*)&mcb->que);
	mcb->own = NULL;
	return  mcb;
}


/***
 *  thread try lock mtx for given amount of time
 */
#if !__FUTEX
tchStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout){
 	tchStatus result = osOK;
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(((tch_mtxDef*)mtx)->key < MTX_INIT_MARK){
			return osErrorParameter;
		}
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
}
#else
tchStatus tch_mtx_lock(tch_mtx_id id,uint32_t timeout){
	tchStatus result = osOK;
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		int tid = (int)Thread->self();
		tch_mtxDef* mtx = (tch_mtxDef*) id;
		if((int)mtx->own == tid)
			return osOK;
		if(mtx->key < MTX_INIT_MARK)
			return osErrorParameter;
		while(tch_port_exclusiveCompareUpdate((int*)&mtx->key,MTX_INIT_MARK,tid | MTX_INIT_MARK)){
			tch_thread_prior prior = Thread->getPriorty((tch_thread_id)tid);
			if(Thread->getPriorty(mtx->own) < prior)
				Thread->setPriority(mtx->own,prior);
			result = tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mtx->que,timeout);
			switch(result){
			case osEventTimeout:
				return osErrorTimeoutResource;
			case osErrorResource:
				return osErrorResource;
			}
		}
		if(result == osOK){
			mtx->own = (void*)tid;
			mtx->svdPrior = Thread->getPriorty((tch_thread_id)tid);
			return osOK;
		}
		tch_kAssert(TRUE,osErrorOS);
		return osErrorOS;
	}
}
#endif


#if !__FUTEX
tchStatus tch_mtx_unlock(tch_mtx_id mtx){
	if(tch_port_isISR()){                               ///< check if in isr mode, then return osErrorISR
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(((tch_mtxDef*)mtx)->key < MTX_INIT_MARK){     ///< otherwise ensure this key is locked by any thread
			return osErrorParameter;                    ///< if not, return osErrorParameter
		}
		return (tchStatus)tch_port_enterSvFromUsr(SV_MTX_UNLOCK,(uint32_t)mtx,0);   ///< otherwise, invoke system call for unlocking mtx
	}
}
#else
tchStatus tch_mtx_unlock(tch_mtx_id id){
	tch_mtxDef* mtx = (tch_mtxDef*) id;
	if(tch_port_isISR()){
		return osErrorISR;
	}else{
		tch_thread_id tid = Thread->self();
		if(mtx->key != ((uint32_t)tid | MTX_INIT_MARK))
			return osErrorResource;
		if(mtx->key < MTX_INIT_MARK)
			return osErrorParameter;
		Thread->setPriority(mtx->own,mtx->svdPrior);
		mtx->svdPrior = Idle;
		mtx->own = NULL;
		mtx->key = MTX_INIT_MARK;
		if(!tch_listIsEmpty(&mtx->que))
			tch_port_enterSvFromUsr(SV_THREAD_RESUME,(uint32_t)&mtx->que,osOK);
		return osOK;
	}
	return osErrorOS;
}
#endif

#if !__FUTEX
tchStatus tch_mtx_destroy(tch_mtx_id mtx){
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
tchStatus tch_mtx_destroy(tch_mtx_id id){
	tch_mtxDef* mtx = (tch_mtxDef*)id;
	if(mtx->key < MTX_INIT_MARK){
		return osErrorResource;
	}
	if(tch_mtx_lock(id,osWaitForever) != osOK)
		return osErrorTimeoutResource;
	mtx->key = 0;
	mtx->own = NULL;
	Thread->setPriority(Thread->self(),mtx->svdPrior);
	mtx->svdPrior = Idle;
	return (tchStatus)tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mtx->que,osErrorResource);
	/*
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		tch_mtxDef* mtx = (tch_mtxDef*) id;
		if(mtx->key < MTX_INIT_MARK){
			return osErrorResource;
		}
		if(mtx->own)
			Thread->setPriority(mtx->own,mtx->svdPrior);   // restore original priority of owner thread
		mtx->key = 0;                                      // set mtx uninitialize
		mtx->own = NULL;
		mtx->svdPrior = Idle;
		return (tchStatus)tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mtx->que,osErrorResource);
	}*/
}
#endif
