/*
 * tch_sys.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 *
 *
 *
 *      this module contains most essential part of tch kernel.
 *
 */


#include "tch_kernel.h"
#include "tch_mem.h"
#include "tch_sched.h"
#include "tch_halcfg.h"
#include "tch_nclib.h"
#include "tch_port.h"
#include "tch_async.h"



typedef struct tch_sysThread_cb_t{
	tch_lnode_t asynTaskQ;
	DECLARE_THREADSTACK(sThread_stack,1 << 10);
	tch_lnode_t sThreadWaitNode;
	tch_thread_id sThread;
}tch_sysThread_cb;

static DECLARE_THREADROUTINE(sThread_routine);

static tch_sysThread_cb sysThread;


static tch_kernel_instance tch_sys_instance;
const tch_kernel_instance* Sys = (const tch_kernel_instance*)&tch_sys_instance;

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
	tch_sys_instance.tch_api.Device = tch_kernel_initHAL();
	if(!tch_sys_instance.tch_api.Device)
		tch_kernel_errorHandler(FALSE,osErrorValue);


	if(!tch_kernel_initPort()){
		tch_kernel_errorHandler(FALSE,osErrorOS);
	}

	tch_ulib_ix* stdlib = tch_initCrt(NULL);


	tch_port_kernel_lock();
#ifndef __USE_MALLOC
	tch_sys_instance.tch_heap_handle = tch_memInit((uint8_t*)&Heap_Base,(uint32_t)&Heap_Limit - (uint32_t)&Heap_Base);
#endif
	tch* api = (tch*) &tch_sys_instance;
	api->Thread = Thread;
	api->Mtx = Mtx;
	api->Sig = Sig;
	api->Mempool = Mempool;
	api->MailQ = MailQ;
	api->MsgQ = MsgQ;
	api->Mem = Mem;

	tch_listInit(&sysThread.sThreadWaitNode);
	tch_thread_cfg th_Cfg;
	th_Cfg._t_name = "SysThread";
	th_Cfg._t_routine = sThread_routine;
	th_Cfg._t_stack = sysThread.sThread_stack;
	th_Cfg.t_proior = Realtime;
	sysThread.sThread = Thread->create(&th_Cfg,api);

	tch_listPutFirst(&sysThread.sThreadWaitNode,((tch_lnode_t*)&sysThread.sThread + 1));

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(&tch_sys_instance);
	return;

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
		tch_schedStartThread((tch_thread_id) arg1);
		return;
	case SV_THREAD_SLEEP:
		tch_schedSleep(arg1,SLEEP);    ///< put current thread in the pend queue and update timeout value in the thread header
		return;
	case SV_THREAD_JOIN:
		tch_schedSuspend((tch_thread_queue*)&((tch_thread_header*)arg1)->t_joinQ,arg2);
		return;
	case SV_THREAD_RESUME:
		nth = tch_schedResume((tch_thread_queue*)arg1);
		nth->t_ctx->kRetv = osOK;
		return;
	case SV_THREAD_RESUMEALL:
		tch_schedResumeAll((tch_thread_queue*)arg1);
		return;
	case SV_THREAD_SUSPEND:
		tch_schedSuspend((tch_thread_queue*)arg1,arg2);
		return;
	case SV_THREAD_TERMINATE:
		cth = (tch_thread_header*) arg1;
		tch_schedTerminate((tch_thread_id) cth,arg2);
		return;
	case SV_MTX_LOCK:
		cth = (tch_thread_header*) tch_schedGetRunningThread();
		if((!(((tch_mtx*) arg1)->key > MTX_INIT_MARK)) || (((tch_mtx*) arg1)->key) == ((uint32_t)cth | MTX_INIT_MARK)){      ///< check mtx is not locked by any thread
			((tch_mtx*) arg1)->key |= (uint32_t) cth;        /// marking mtx key as locked
			if(!cth->t_lckCnt++){                            /// ensure priority escalation occurs only once
				cth->t_svd_prior = cth->t_prior;
				cth->t_prior = Unpreemtible;                 /// if thread owns mtx, its priority is escalated to unpreemptible level
				                                             ///
				                                             /// This temporary priority change is to minimize resource allocation time
				                                             /// of single thread
				                                             ///
			}
			sp->R0 = osOK;
			return;
		}else{                                                 ///< otherwise, make thread block
			tch_schedSuspend((tch_thread_queue*)&((tch_mtx*) arg1)->que,arg2);
		}
		return;
	case SV_MTX_UNLOCK:
		if(((tch_mtx*) arg1)->key > MTX_INIT_MARK){
			if(!--cth->t_lckCnt){
				cth->t_prior = cth->t_svd_prior;
			}
			nth = tch_schedResume((tch_thread_queue*)&((tch_mtx*)arg1)->que);
			if(nth){
				((tch_mtx*) arg1)->key |= (uint32_t)nth;
				nth->t_ctx->kRetv = osOK;
			}else{
				((tch_mtx*) arg1)->key = MTX_INIT_MARK;
			}
			sp->R0 = osOK;
		}else{
			sp->R0 = osErrorResource;    ///< this mutex is not initialized yet.
		}
		return;
	case SV_MTX_DESTROY:
		while(!tch_listIsEmpty(&((tch_mtx*)arg1)->que)){               ///< check waiting thread in mtx entry
			nth = (tch_thread_header*) tch_listDequeue((tch_lnode_t*)&((tch_mtx*) arg1)->que);
			nth = (tch_thread_header*) ((tch_lnode_t*) nth - 1);
			nth->t_ctx->kRetv = osErrorResource;
			nth->t_waitQ = NULL;
			tch_schedReady(nth);                    ///< if there is thread waiting,put it ready state
		}
		return;
	case SV_SIG_WAIT:
		cth = (tch_thread_header*) tch_schedGetRunningThread();
		cth->t_sig.match_target = arg1;                         ///< update thread signal pattern
		tch_schedSuspend((tch_thread_queue*)&cth->t_sig.sig_wq,arg2);///< suspend to signal wq
		return;
	case SV_SIG_MATCH:
		cth = (tch_thread_header*) arg1;
		nth = tch_schedResume((tch_thread_queue*)&cth->t_sig.sig_wq);
		nth->t_ctx->kRetv = osOK;
		return;
	case SV_MEM_MALLOC:
		sp->R0 = (uint32_t)Sys->tch_heap_handle->alloc(Sys->tch_heap_handle,arg1);
		return;
	case SV_MEM_FREE:
		sp->R0 = Sys->tch_heap_handle->free(Sys->tch_heap_handle,(void*)arg1);
		return;
	case SV_ASYNC_START:
		tch_listEnqueuePriority(&sysThread.asynTaskQ,(tch_lnode_t*)arg1,tch_async_comp);
		tch_schedResume(&sysThread.sThreadWaitNode,osOK);
		return;
	case SV_ASYNC_BLSTART:
		tch_listEnqueuePriority(&sysThread.asynTaskQ,(tch_lnode_t*)arg1,tch_async_comp);
		if(!tch_listIsEmpty(&sysThread.sThreadWaitNode))
			tch_schedReady((tch_thread_id)tch_listDequeue(&sysThread.sThreadWaitNode));
		tch_schedSuspend(&((tch_async_cb*)arg1)->wq,arg2);
		return;
	case SV_ASYNC_NOTIFY:
		tch_schedResume(&((tch_async_cb*) arg1)->wq,arg2);
	}
}



static DECLARE_THREADROUTINE(sThread_routine){
	//initiailize task q
	tch_listInit(&sysThread.asynTaskQ);
	while(TRUE){
		while(tch_listIsEmpty(&sysThread.asynTaskQ)){ // perform tasks
			tch_async_cb* cb = tch_listDequeue(&sysThread.asynTaskQ);
			cb->fn(cb,cb->arg);
		}
		tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,&sysThread.sThreadWaitNode,0);  // suspend until new task available
	}
	return osOK;
}



void tch_kernel_errorHandler(BOOL dump,tchStatus status){

	/**
	 *  optional dump register
	 */
	while(1){
		asm volatile("NOP");
	}
}



void tch_kernel_faulthandle(int fault){
	switch(fault){
	case FAULT_TYPE_BUS:
		break;
	case FAULT_TYPE_HARD:
		break;
	case FAULT_TYPE_MEM:
		break;
	case FAULT_TYPE_USG:
		break;
	}
	while(1){
		asm volatile("NOP");
	}
}


