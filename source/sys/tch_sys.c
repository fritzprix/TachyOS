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
#include "tch_sys.h"
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
	tch_threadId sThread;
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


	tch_port_kernel_lock();
#ifndef __USE_MALLOC
	Heap_Manager = tch_memInit((uint8_t*)&Heap_Base,(uint32_t)&Heap_Limit - (uint32_t)&Heap_Base);
#endif
	/*Bind API Object*/
	tch* api = (tch*) &tch_sys_instance;
	api->uStdLib = tch_initCrt(NULL);
	api->Thread = Thread;
	api->Mtx = Mtx;
	api->Sem = Sem;
	api->Condv = Condv;
	api->Barrier = Barrier;
	api->Sig = Sig;
	api->Mempool = Mempool;
	api->MailQ = MailQ;
	api->MsgQ = MsgQ;
	api->Mem = Mem;
	api->Async = Async;

	tch_listInit(&sysThread.sThreadWaitNode);
	tch_thread_cfg th_Cfg;
	th_Cfg._t_name = "SysThread";
	th_Cfg._t_routine = sThread_routine;
	th_Cfg._t_stack = sysThread.sThread_stack;
	th_Cfg.t_proior = KThread;
	th_Cfg.t_stackSize = (1 << 10);
	sysThread.sThread = Thread->create(&th_Cfg,api);

	tch_listPutFirst(&sysThread.sThreadWaitNode,((tch_lnode_t*)sysThread.sThread + 1));

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(&tch_sys_instance);
	return;

}



void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_thread_header* cth = NULL;
	tch_thread_header* nth = NULL;
	tch_exc_stack* sp = NULL;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp = (tch_exc_stack*)tch_port_getThreadSP();
		sp++;
		tch_port_setThreadSP((uint32_t)sp);
		tch_port_kernel_unlock();
		return;
	case SV_THREAD_START:              // start thread first time
		tch_schedStartThread((tch_threadId) arg1);
		return;
	case SV_THREAD_SLEEP:
		tch_schedSleep(arg1,SLEEP);    // put current thread in pending queue and will be waken up at given after given time duration is passed
		return;
	case SV_THREAD_JOIN:
		if(((tch_thread_header*)arg1)->t_state != TERMINATED){                                 // check target if thread has terminated
			tch_schedSuspend((tch_thread_queue*)&((tch_thread_header*)arg1)->t_joinQ,arg2);    //if not, thread wait
			return;
		}
		tch_kernelSetResult(tch_currentThread,osOK);                                           //..otherwise, it returns immediately
		return;
	case SV_THREAD_RESUME:
		tch_schedResume((tch_thread_queue*)arg1,arg2);
		return;
	case SV_THREAD_RESUMEALL:
		tch_schedResumeAll((tch_thread_queue*)arg1,arg2,TRUE);
		return;
	case SV_THREAD_SUSPEND:
		tch_schedSuspend((tch_thread_queue*)arg1,arg2);
		return;
	case SV_THREAD_TERMINATE:
		cth = (tch_thread_header*) arg1;
		tch_schedTerminate((tch_threadId) cth,arg2);
		return;
	case SV_SIG_WAIT:
		cth = (tch_thread_header*) tch_schedGetRunningThread();
		cth->t_sig.match_target = arg1;                         ///< update thread signal pattern
		tch_schedSuspend((tch_thread_queue*)&cth->t_sig.sig_wq,arg2);///< suspend to signal wq
		return;
	case SV_SIG_MATCH:
		cth = (tch_thread_header*) arg1;
		nth = tch_schedResume((tch_thread_queue*)&cth->t_sig.sig_wq,osOK);
		return;
	case SV_MEM_MALLOC:
#ifndef __USE_MALLOC
		tch_kernelSetResult(tch_currentThread,(uint32_t)Heap_Manager->alloc(Heap_Manager,arg1));
#endif
		return;
	case SV_MEM_FREE:
#ifndef __USE_MALLOC
		tch_currentThread = Heap_Manager->free(Heap_Manager,(void*)arg1);
#endif
		return;
	case SV_ASYNC_START:
		if(!((tch_async_cb*)arg1)->fn){
			tch_kernelSetResult(tch_currentThread,osOK);
			return;
		}
		tch_listEnqueuePriority(&sysThread.asynTaskQ,(tch_lnode_t*)arg1,tch_async_comp);
		tch_schedResume((uaddr_t)&sysThread.sThreadWaitNode,osOK);
		return;
	case SV_ASYNC_BLSTART:
		if(((tch_async_cb*) arg1)->fn){
			tch_listEnqueuePriority(&sysThread.asynTaskQ,(tch_lnode_t*)arg1,tch_async_comp);
			if(!tch_listIsEmpty(&sysThread.sThreadWaitNode))
				tch_schedReady((tch_threadId)tch_listDequeue(&sysThread.sThreadWaitNode));
		}
		tch_schedSuspend(&((tch_async_cb*)arg1)->wq,arg2);
		return;
	case SV_ASYNC_NOTIFY:
		tch_schedResume(&((tch_async_cb*) arg1)->wq,arg2);
		return;
	case SV_MSGQ_PUT:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kput(arg1,arg2));
		return;
	case SV_MSGQ_GET:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kget(arg1,arg2));
		return;
	case SV_MSGQ_DESTROY:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kdestroy(arg1));
		return;
	}
}



static DECLARE_THREADROUTINE(sThread_routine){
	//initiailize task q
	tch_listInit(&sysThread.asynTaskQ);
	while(TRUE){
		while(!tch_listIsEmpty(&sysThread.asynTaskQ)){ // perform tasks
			tch_async_cb* cb = tch_listDequeue(&sysThread.asynTaskQ);
			cb->fn(cb,cb->arg);
		}
		tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,&sysThread.sThreadWaitNode,osWaitForever);  // suspend until new task available why osWaitfor
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


