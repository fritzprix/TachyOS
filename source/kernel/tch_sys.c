/*
 * tch_sys.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 *
 *
 *
 *      this module contains most essential part of tch kernel.
 *
 */


#include "kernel/tch_kernel.h"
#include "kernel/tch_sched.h"





static tch_kernel_instance tch_sys_instance;



/***
 *  Initialize Kernel including...
 *  - initailize device driver and bind HAL interface
 *  - initialize architecture dependent part and bind port interface
 *  - bind User APIs to API type
 *  - initialize Idle thread
 */
void tch_kernelInit(void* arg){

	/**
	 *  dynamic binding of dependecy
	 */
	tch_sys_instance.tch_api.Device = tch_hal_init();
	if(!tch_sys_instance.tch_api.Device)
		tch_error_handler(false,osErrorValue);


	if(!tch_port_init()){
		tch_error_handler(false,osErrorOS);
	}

	tch_port_kernel_lock();

	tch* api = &tch_sys_instance.tch_api;
	api->Thread = Thread;
	api->Mtx = Mtx;


	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(&tch_sys_instance);

}

void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_exc_stack* sp = (tch_exc_stack*)tch_port_getThreadSP();
	tch_thread_header* cth = NULL;
	tch_thread_header* nth = NULL;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp++;
		tch_port_setThreadSP((uint32_t)sp);
		sp->R0 = arg1;                ///< return kernel call result
		tch_port_kernel_unlock();
		return;
	case SV_THREAD_START:             // thread pointer arg1
		sp->R0 = tch_schedStartThread((tch_thread_id) arg1);
		return;
	case SV_THREAD_SLEEP:
		sp->R0 = tch_schedScheduleToSuspend(arg1,SLEEP);    ///< put current thread in the pend queue and update timeout value in the thread header
		return;
	case SV_THREAD_JOIN:
		return;
	case SV_MTX_LOCK:
		if(!(((tch_mtx*) arg1)->key > MTX_INIT_MARK)){
			cth = (tch_thread_header*) tch_schedGetRunningThread();
			((tch_mtx*) arg1)->key |= (uint32_t) cth;
			if(!cth->t_lckCnt++){                            ///< ensure priority escalation occurs only once
				cth->t_svd_prior = cth->t_prior;
				cth->t_prior = Unpreemtible;
			}
			sp->R0 = osOK;
			return;
		}else{
			sp->R0 = tch_schedScheduleToWaitTimeout((tch_genericList_queue_t*)&((tch_mtx*) arg1)->que,arg2);
		}
		return;
	case SV_MTX_UNLOCK:
		if(((tch_mtx*) arg1)->key > MTX_INIT_MARK){
			if(((tch_mtx*) arg1)->que.que.entry){
				nth = (tch_thread_header*) tch_genericQue_dequeue((tch_genericList_queue_t*)&((tch_mtx*) arg1)->que);
				nth = (tch_thread_header*)((tch_genericList_node_t*) nth - 1);
				((tch_mtx*) arg1)->key |= (uint32_t)nth;
				tch_schedScheduleToReady(nth);
			}else{
				((tch_mtx*) arg1)->key = MTX_INIT_MARK;
			}
			cth = (tch_thread_header*) tch_schedGetRunningThread();
			if(!--cth->t_lckCnt){
				cth->t_prior = cth->t_svd_prior;
			}
			sp->R0 = osOK;
		}else{
			sp->R0 = osErrorResource;
		}
		return;
	}
}




void tch_error_handler(BOOL dump,osStatus status){

	/**
	 *  optional dump register
	 */
	while(1){
		asm volatile("NOP");
	}
}



