/*
 * fmo_synch.c
 *
 *  Created on: 2014. 4. 2.
 *      Author: innocentevil
 */


#include "fmo_synch.h"
#include "fmo_sched.h"




void tch_Mtx_init(tch_mtx_lock* lock){
	lock->key = (uint32_t) NULL;
	tch_genericQue_Init((tch_genericList_queue_t*)&lock->w_q);
}


BOOL tch_Mtx_lockt(tch_mtx_lock* lock,BOOL wait){
	if(wait){
		while(1){
			if(!__get_IPSR()){
				__sv_call(SVC_MTX_LOCK,(uint32_t) lock,wait);
			}else{
				__pndsv_call(SVC_MTX_LOCK,(uint32_t)lock,wait);
			}
			if(lock->key == (uint32_t)tchThread_getCurrent()){                                                                 //// check whether thread lock mtx and return,otherwise retry lock mtx
				return TRUE;
			}
		}
	}else{
		if(!__get_IPSR()){
			return __sv_call(SVC_MTX_LOCK,(uint32_t) lock,wait);
		}else{
			return FALSE;
		}
	}
	return FALSE;
}

void tch_Mtx_unlockt(tch_mtx_lock* lock){
	if(!__get_IPSR()){
		__sv_call(SVC_MTX_UNLOCK,(uint32_t)lock,0);
	}else{
		__pndsv_call(SVC_MTX_UNLOCK,(uint32_t) lock,0);
	}
}


void tch_Sem_init(tch_sem_lock* sem,uint32_t maxcnt){

}

BOOL tch_Sem_up(tch_sem_lock* sem, BOOL wait){

}

BOOL tch_Sem_down(tch_sem_lock* sem, BOOL wait){

}

void tch_Sem_deinit(tch_sem_lock* sem){

}
