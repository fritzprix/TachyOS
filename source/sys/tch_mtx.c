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



static tch_mtx_id tch_mtx_create(tch_mtx* mtx);
static osStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout);
static osStatus tch_mtx_unlock(tch_mtx_id mtx);
static osStatus tch_mtx_destroy(tch_mtx_id mtx);



__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mtx_create,
		tch_mtx_lock,
		tch_mtx_unlock,
		tch_mtx_destroy,
};

const tch_mtx_ix* Mtx = &MTX_Instance;



tch_mtx_id tch_mtx_create(tch_mtx* mtx){
	mtx->key = MTX_INIT_MARK;
	tch_genericQue_Init((tch_genericList_queue_t*)&mtx->que);
	return (tch_mtx_id) mtx;
}


/***
 *  thread try lock mtx for given amount of time
 */
osStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(false,osErrorISR);
		return osErrorISR;
	}else{
		if(getMtxObject(mtx)->key < MTX_INIT_MARK){
			return osErrorParameter;
		}
		while(true){
			switch(tch_port_enterSvFromUsr(SV_MTX_LOCK,(uint32_t)mtx,timeout)){
			case osErrorTimeoutResource:
				return osErrorTimeoutResource;
			case osErrorResource:
				return osErrorResource;
			case osOK:
				if(getMtxObject(mtx)->key >= (uint32_t)tch_schedGetRunningThread()){
					return osOK;
				}
			}
		}
		return osErrorResource;
	}
}

osStatus tch_mtx_unlock(tch_mtx_id mtx){
	if(tch_port_isISR()){                               ///< check if in isr mode, then return osErrorISR
		tch_kernel_errorHandler(false,osErrorISR);
		return osErrorISR;
	}else{
		if(getMtxObject(mtx)->key < MTX_INIT_MARK){     ///< otherwise ensure this key is locked by any thread
			return osErrorParameter;                    ///< if not, return osErrorParameter
		}
		return tch_port_enterSvFromUsr(SV_MTX_UNLOCK,(uint32_t)mtx,0);   ///< otherwise, invoke system call for unlocking mtx
	}
}

osStatus tch_mtx_destroy(tch_mtx_id mtx){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(false,osErrorISR);
		return osErrorISR;
	}else{
		if(!(getMtxObject(mtx)->key > MTX_INIT_MARK)){
			return osErrorResource;
		}
		getMtxObject(mtx)->key = 0;
		return tch_port_enterSvFromUsr(SV_MTX_DESTROY,(uint32_t)mtx,0);
	}
}
