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
#include "tch_syscfg.h"




static DECLARE_THREADROUTINE(systhreadRoutine);
static DECLARE_THREADROUTINE(idle);


static tch RuntimeInterface;
const tch* tch_rti = &RuntimeInterface;
tch_mailqId sysTaskQ;
tch_memId sharedMem;



/***
 *  Initialize Kernel including...
 *  - initailize device driver and bind HAL interface
 *  - initialize architecture dependent part and bind port interface
 *  - bind User APIs to API type
 *  - initialize Idle thread
 */
void tch_kernelInit(void* arg){

	tch_threadId sysThread = NULL;

	/*Bind API Object*/

	RuntimeInterface.uStdLib = tch_initStdLib();

	RuntimeInterface.Thread = Thread;
	RuntimeInterface.Mtx = Mtx;
	RuntimeInterface.Sem = Sem;
	RuntimeInterface.Condv = Condv;
	RuntimeInterface.Barrier = Barrier;
	RuntimeInterface.Sig = Sig;
	RuntimeInterface.Mempool = Mempool;
	RuntimeInterface.MailQ = MailQ;
	RuntimeInterface.MsgQ = MsgQ;
	RuntimeInterface.Mem = uMem;

	tch_listInit(&tch_procList);

	uint8_t* shMemBlk = kMem->alloc(TCH_CFG_SHARED_MEM_SIZE);
	sharedMem = tch_memCreate(shMemBlk,TCH_CFG_SHARED_MEM_SIZE);


	/**
	 *  dynamic binding of dependecy
	 */
	RuntimeInterface.Device = tch_kernel_initHAL();
	if(!RuntimeInterface.Device)
		tch_kernel_errorHandler(FALSE,osErrorValue);


	if(!tch_kernel_initPort()){
		tch_kernel_errorHandler(FALSE,osErrorOS);
	}

	tch_port_kernel_lock();
	// create system task thread

	tch_threadCfg thcfg;
	thcfg._t_name = "sysloop";
	thcfg._t_routine = systhreadRoutine;
	thcfg.t_proior = KThread;
	thcfg.t_stackSize = 1 << 11;
	tch_currentThread = ROOT_THREAD;
	sysThread = Thread->create(&thcfg,(void*)tch_rti);

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(sysThread);
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
		_impure_ptr = &tch_currentThread->t_reent;
		tch_port_setThreadSP((uint32_t)sp);
		if(tch_currentThread->t_state == DESTROYED)
			tch_schedDestroy(tch_currentThread,tch_currentThread->t_kRet);
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
	case SV_THREAD_DESTROY:
		cth = (tch_thread_header*) arg1;
		tch_schedDestroy((tch_threadId) cth,arg2);
		break;
	case SV_SIG_WAIT:
		cth = (tch_thread_header*) tch_schedGetRunningThread();
		cth->t_sig.sig_comb = arg1;                         ///< update thread signal pattern
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
	}
}

tchStatus tch_kernel_postSysTask(int id,tch_sysTaskFn fn,void* arg){
	tchStatus result = osOK;
	tch_sysTask* task = tch_rti->MailQ->alloc(sysTaskQ,osWaitForever,&result);
	task->arg = arg;
	task->fn = fn;
	task->id = id;
	task->prior = tch_currentThread->t_prior;
	task->status = osOK;
	if(result == osOK)
		tch_rti->MailQ->put(sysTaskQ,task);
	return result;
}



void tch_kernel_errorHandler(BOOL dump,tchStatus status){

	/**
	 *  optional dump register
	 */
	while(1){
		asm volatile("NOP");
	}
}

static DECLARE_THREADROUTINE(systhreadRoutine){
	// perform runtime initialization
	tch_threadId mainThread = NULL;
	tch_threadId idleThread = NULL;

	osEvent evt;
	tch_sysTask* task = NULL;



	if(tch_kernel_initCrt0(&RuntimeInterface) != osOK)
		tch_kernel_errorHandler(TRUE,osErrorOS);

	// initialize sys thread mail box for task queueing
	sysTaskQ = MailQ->create(sizeof(tch_sysTask),TCH_SYS_TASKQ_SZ);
	if(!sysTaskQ)
		tch_kernel_errorHandler(TRUE,osErrorOS);

	tch_threadId th = tch_currentThread;
	tch_currentThread = ROOT_THREAD;      // create thread as root

	tch_threadCfg thcfg;
	thcfg._t_routine = (tch_thread_routine) main;
	thcfg.t_stackSize = (uint32_t) &Main_Stack_Top - (uint32_t) &Main_Stack_Limit;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";
	mainThread = Thread->create(&thcfg,&RuntimeInterface);
	tch_currentThread = th;

	thcfg._t_routine = (tch_thread_routine) idle;
	thcfg.t_stackSize = (uint32_t)&Idle_Stack_Top - (uint32_t)&Idle_Stack_Limit;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	idleThread = Thread->create(&thcfg,&RuntimeInterface);


	if((!mainThread) || (!idleThread))
		tch_kernel_errorHandler(TRUE,osErrorOS);

	Thread->start(idleThread);
	Thread->start(mainThread);

	uStdLib->string->memset(&evt,0,sizeof(osEvent));

	uStdLib->stdio->iprintf("\rUser Heap Top:  %x\n",&Heap_Limit);
	uStdLib->stdio->iprintf("\rUser Heap Bottom : %x\n",&Heap_Base);
	uStdLib->stdio->iprintf("\rThread Header Size : %d\n",sizeof(tch_thread_header));



	// loop for handling system tasks (from ISR / from any user thread)
	while(TRUE){
		evt = MailQ->get(sysTaskQ,osWaitForever);
		if(evt.status == osEventMail){
			task = (tch_sysTask*) evt.value.p;
			task->fn(task->id,&RuntimeInterface,task->arg);
			MailQ->free(sysTaskQ,evt.value.p);
		}
	}
	return osOK;    // unreachable point

}


static DECLARE_THREADROUTINE(idle){


	while(TRUE){
		// some function entering sleep mode
		__DMB();
		__ISB();
		__WFI();
		__DMB();
		__ISB();
		// some function waking up from sleep mode
	}
	return 0;
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



