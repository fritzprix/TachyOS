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

/*
tchStatus tch_mtx_lock(tch_mtx* mtx,uint32_t timeout){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(mtx->key < MTX_INIT_MARK)
			return osErrorParameter;
		while(TRUE){
			int tid = (int)Thread->self();
			if(!tch_port_exclusiveCompare(&mtx->key,tid,tid)){
				return osOK;
			}

			if(tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,&mtx->que,timeout) != osOK){
				return osErrorTimeoutResource;
			}
		}
		return osErrorResource;
	}
}*/

/*
tchStatus tch_mtx_unlock(tch_mtx* mtx){
	if(tch_port_isISR()){
		return osErrorISR;
	}else{
		if(mtx->key != Thread->self())
			return osErrorParameter;
		if(mtx->key == MTX_INIT_MARK)
			return osErrorResource;
		if(tch_port_exclusiveCompare(&mtx->key,MTX_INIT_MARK,MTX_INIT_MARK)){
			tch_port_enterSvFromUsr(SV_THREAD_RESUME,&mtx->que,0);
		}
	}
}
*/


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
/*
tchStatus tch_mtx_destroy(tch_mtx* mtx){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(!(mtx->key > MTX_INIT_MARK)){
			return osErrorResource;
		}
		mtx->key = 0;
		return (tchStatus)tch_port_enterSvFromUsr(SV_MTX_DESTROY,(uint32_t)mtx,0);
	}
}*/
