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
#include "tch_halcfg.h"
#include "tch_nclib.h"
#include "tch_port.h"
#include "tch_async.h"






static tch_sys_instance Sys_StaticInstance;
const tch_sys_instance* Sys = (const tch_sys_instance*)&Sys_StaticInstance;

/***
 *  Initialize Kernel including...
 *  - initailize device driver and bind HAL interface
 *  - initialize architecture dependent part and bind port interface
 *  - bind User APIs to API type
 *  - initialize Idle thread
 */
void tch_kernelInit(void* arg){


	/*Bind API Object*/
	tch* api = (tch*) &Sys_StaticInstance;

	api->uStdLib = tch_initStdLib();
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
	api->pTask = tch_initpTask(api);
//	api->Async = Async;



	/**
	 *  dynamic binding of dependecy
	 */
	Sys_StaticInstance.tch_api.Device = tch_kernel_initHAL();
	if(!Sys_StaticInstance.tch_api.Device)
		tch_kernel_errorHandler(FALSE,osErrorValue);


	if(!tch_kernel_initPort()){
		tch_kernel_errorHandler(FALSE,osErrorOS);
	}

	tch_port_kernel_lock();
	// create system task thread


	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(&Sys_StaticInstance);
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
		break;
	case SV_THREAD_START:              // start thread first time
		tch_schedStartThread((tch_threadId) arg1);
		break;
	case SV_THREAD_SLEEP:
		tch_schedSleep(arg1,SLEEP);    // put current thread in pending queue and will be waken up at given after given time duration is passed
		break;
	case SV_THREAD_JOIN:
		if(((tch_thread_header*)arg1)->t_state != TERMINATED){                                 // check target if thread has terminated
			tch_schedSuspend((tch_thread_queue*)&((tch_thread_header*)arg1)->t_joinQ,arg2);    //if not, thread wait
			break;
		}
		tch_kernelSetResult(tch_currentThread,osOK);                                           //..otherwise, it returns immediately
		break;
	case SV_THREAD_RESUME:
		tch_schedResumeM((tch_thread_queue*) arg1,1,arg2,TRUE);
		break;
	case SV_THREAD_RESUMEALL:
		tch_schedResumeM((tch_thread_queue*) arg1,SCHED_THREAD_ALL,arg2,TRUE);
		break;
	case SV_THREAD_SUSPEND:
		tch_schedSuspend((tch_thread_queue*)arg1,arg2);
		break;
	case SV_THREAD_TERMINATE:
		cth = (tch_thread_header*) arg1;
		tch_schedTerminate((tch_threadId) cth,arg2);
		break;
	case SV_SIG_WAIT:
		cth = (tch_thread_header*) tch_schedGetRunningThread();
		cth->t_sig.match_target = arg1;                         ///< update thread signal pattern
		tch_schedSuspend((tch_thread_queue*)&cth->t_sig.sig_wq,arg2);///< suspend to signal wq
		break;
	case SV_SIG_MATCH:
		cth = (tch_thread_header*) arg1;
		tch_schedResumeM((tch_thread_queue*)&cth->t_sig.sig_wq,SCHED_THREAD_ALL,osOK,TRUE);
		break;
	case SV_MSGQ_PUT:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kput((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_GET:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kget((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_DESTROY:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_msgq_kdestroy((tch_msgqId) arg1));
		break;
	case SV_MAILQ_ALLOC:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kalloc((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_FREE:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kfree((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_DESTROY:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,tch_mailq_kdestroy((tch_mailqId) arg1,0));
		break;
	case SV_ASYNC_WAIT:
		cth = tch_currentThread;
//		tch_kernelSetResult(cth,tch_async_kwait(arg1,arg2,&sysTaskQue));
		break;
	case SV_ASYNC_NOTIFY:
		cth = tch_currentThread;
//		tch_kernelSetResult(cth,tch_async_knotify(arg1,arg2));
		break;
	case SV_ASYNC_DESTROY:
		cth = tch_currentThread;
//		tch_kernelSetResult(cth,tch_async_kdestroy(arg1));
		break;
	case SV_UNIX_SBRK:
		cth = tch_currentThread;
		tch_kernelSetResult(cth,(tchStatus)tch_sbrk_k((void*)arg1,arg2));
		if(cth->t_kRet == (uint32_t)NULL)
			tch_schedTerminate(cth,osErrorNoMemory);
		break;
	}
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


