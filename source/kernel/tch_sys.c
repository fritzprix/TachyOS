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



#include "tch.h"

#include "tch_hal.h"
#include "tch_kernel.h"
#include "tch_mailq.h"
#include "tch_msgq.h"
#include "tch_mem.h"
#include "tch_nclib.h"
#include "tch_port.h"
#include "tch_thread.h"
#include "tch_time.h"
#include "tch_board.h"

#define SYSTSK_ID_SLEEP             ((int) -3)
#define SYSTSK_ID_ERR_HANDLE        ((int) -2)
#define SYSTSK_ID_RESET             ((int) -1)

typedef struct tch_busy_monitor_t {
	tch_mtxId 	mtx;
	uint32_t 	wrk_load;
}tch_busy_monitor;


static DECLARE_THREADROUTINE(systhreadRoutine);

static tch_busy_monitor busyMonitor;
static tch RuntimeInterface;
static tch_threadId mainThread = NULL;
static tch_mailqId sysTaskQ;
static tch_threadId sysThread;

tch_thread_queue procList;
tch_boardParam boardHandle = NULL;
BOOL __VALID_SYSCALL;

const tch* tch_rti = &RuntimeInterface;


/***
 *  Kernel initailization is followed sequence below
 *  1. initialize kernel stack and dyanmic memory region @ tch_kernelMemInit()
 *  2. perform cpu specific initialization
 */
void tch_kernelInit(void* arg){

	tch_kernelMemInit(NULL);
	if(!tch_kernelInitPort())										// initialize port layer
		tch_kernel_errorHandler(FALSE,tchErrorOS);


	/*initialize kernel global variable*/
	cdsl_dlistInit((cdsl_dlistNode_t*) &procList);


	__VALID_SYSCALL = FALSE;
	/*Bind API Object*/


	RuntimeInterface.Thread = Thread;
	RuntimeInterface.Mtx = Mtx;
	RuntimeInterface.Sem = Sem;
	RuntimeInterface.Condv = Condv;
	RuntimeInterface.Barrier = Barrier;
	RuntimeInterface.Mempool = Mempool;
	RuntimeInterface.MailQ = MailQ;
	RuntimeInterface.MsgQ = MsgQ;
	RuntimeInterface.Mem = uMem;
	RuntimeInterface.Event = Event;

	mainThread = NULL;
	sysThread = NULL;

	/**
	 *  Initialize pageing sub-system for memory managment
	 */
	if(tchk_pageInit(&Sys_Heap_Base,((uword_t)&Sys_Heap_Limit - (uword_t)&Sys_Heap_Base)) != tchOK)
		tch_kernel_errorHandler(FALSE,tchErrorOS);

	tchk_shareableMemInit(TCH_CFG_SHARED_MEM_SIZE);					// Initialize shareable(publicly accessable from all execution context) memory allocator
	tchk_kernelHeapInit(TCH_CFG_KERNEL_HEAP_MEM_SIZE);				// Initialize kernel heap allocator(only accessible from privilidged level)

	tch_port_atomic_begin();

	tch_threadCfg thcfg;
	Thread->initCfg(&thcfg);
	thcfg.t_name = "systhread";
	thcfg.t_routine = systhreadRoutine;
	thcfg.t_priority = Kernel;
	thcfg.t_memDef.stk_sz = 1 << 10;
	sysThread = tchk_threadCreateThread(&thcfg,(void*) tch_rti,TRUE,TRUE);

	tch_schedInit(sysThread);
	return;
}


void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	if(__VALID_SYSCALL)
		__VALID_SYSCALL = FALSE;
	else
		tch_kernel_errorHandler(FALSE,tchErrorOS);


	tch_thread_kheader* cth = NULL;
	tch_thread_kheader* nth = NULL;
	tch_exc_stack* sp = NULL;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp = (tch_exc_stack*)tch_port_getUserSP();
		sp++;
		cth = (tch_thread_kheader*) tch_currentThread->t_kthread;
		cth->t_tslot = 0;
		cth->t_state = RUNNING;

#ifdef __NEWLIB__
		_impure_ptr = &tch_currentThread->t_reent;
#endif

		tchk_mapPage(cth->t_pgId);			/// apply page mapping
		tch_port_setUserSP((uint32_t)sp);
		if((arg1 = tchk_threadIsValid(tch_currentThread)) == tchOK){
			tch_port_atomic_end();
		}else{
			tch_schedThreadDestroy(tch_currentThread,arg1);
		}
		if(tchk_threadIsPrivilidged(tch_currentThread))
			tch_port_enable_privilegedThread();
		else
			tch_port_disable_privilegedThread();
		break;
	case SV_CONDV_INIT:
		tchk_kernelSetResult(tch_currentThread,tchk_condvInit(arg1,arg2));
		break;
	case SV_CONDV_WAIT:

	case SV_CONDV_WAKE:
	case SV_CONDV_DEINIT:
		break;
	case SV_BAR_INIT:
		tchk_kernelSetResult(tch_currentThread,tchk_barrierInit(arg1,arg2));
		break;
	case SV_BAR_DEINIT:
		tchk_kernelSetResult(tch_currentThread,tchk_barrierDeinit(arg1));
		break;
	case SV_SHMEM_ALLOC:
		tchk_kernelSetResult(tch_currentThread,tchk_shareableMemAlloc(arg1,arg2));
		break;
	case SV_SHMEM_FREE:
		tchk_kernelSetResult(tch_currentThread,tchk_shareableMemFree(arg1));
		break;
	case SV_SHMEM_AVAILABLE:
		tchk_kernelSetResult(tch_currentThread,tchk_shareableMemAvail(arg1));
		break;
	case SV_THREAD_CREATE:
		tchk_kernelSetResult(tch_currentThread,(uword_t) tchk_threadCreateThread((tch_threadCfg*) arg1,(void*) arg2,FALSE,FALSE));
		break;
	case SV_THREAD_START:              // start thread first time
		tchk_schedThreadStart((tch_threadId) arg1);
		break;
	case SV_THREAD_YIELD:
		tchk_schedThreadSleep(arg1,mSECOND,WAIT);    // put current thread in pending queue and will be waken up at given after given time duration is passed
		break;
	case SV_THREAD_SLEEP:
		tchk_schedThreadSleep(arg1,SECOND,SLEEP);
		break;
	case SV_THREAD_JOIN:
		nth = ((tch_thread_uheader*) arg1)->t_kthread;
		if(nth->t_state != TERMINATED){                                 // check target if thread has terminated
			tchk_schedThreadSuspend((tch_thread_queue*) &nth->t_joinQ,arg2);   				 //if not, thread wait
			break;
		}
		tchk_kernelSetResult(tch_currentThread,tchOK);                                           //..otherwise, it returns immediately
		break;
	case SV_THREAD_RESUME:
		tchk_schedThreadResumeM((tch_thread_queue*) arg1,1,arg2,TRUE);
		break;
	case SV_THREAD_RESUMEALL:
		tchk_schedThreadResumeM((tch_thread_queue*) arg1,SCHED_THREAD_ALL,arg2,TRUE);
		break;
	case SV_THREAD_SUSPEND:
		tchk_schedThreadSuspend((tch_thread_queue*)arg1,arg2);
		break;
	case SV_THREAD_TERMINATE:
		tchk_schedThreadTerminate((tch_threadId) arg1,arg2);
		break;
	case SV_THREAD_DESTROY:
		tch_schedThreadDestroy((tch_threadId) arg1,arg2);
		break;
	case SV_MTX_LOCK:
		tchk_kernelSetResult(tch_currentThread,tchk_mutex_lock(arg1,arg2));
		break;
	case SV_MTX_UNLOCK:
		tchk_kernelSetResult(tch_currentThread,tchk_mutex_unlock(arg1));
		break;
	case SV_MTX_DESTROY:
		tchk_kernelSetResult(tch_currentThread,tchk_mutex_destroy(arg1));
		break;
	case SV_EV_INIT:
		tchk_kernelSetResult(tch_currentThread,tchk_eventInit(arg1,arg2));
		break;
	case SV_EV_WAIT:
		tchk_kernelSetResult(tch_currentThread,tchk_eventWait(arg1,arg2));
		break;
	case SV_EV_UPDATE:
		tchk_kernelSetResult(tch_currentThread,tchk_eventUpdate(arg1,arg2));
		break;
	case SV_EV_DEINIT:
		tchk_kernelSetResult(tch_currentThread,tchk_eventDeinit(arg1));
		break;
	case SV_MSGQ_INIT:
		tchk_kernelSetResult(tch_currentThread,(uword_t)tchk_msgqInit((tch_msgqId) arg1,arg2));
		break;
	case SV_MSGQ_PUT:
		tchk_kernelSetResult(tch_currentThread,tchk_msgqPut((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_GET:
		tchk_kernelSetResult(tch_currentThread,tchk_msgqGet((tch_msgqId) arg1,(void*) arg2));
		break;
	case SV_MSGQ_DEINIT:
		tchk_kernelSetResult(tch_currentThread,tchk_msgqDeinit((tch_msgqId) arg1));
		break;
	case SV_MAILQ_INIT:
		tchk_kernelSetResult(tch_currentThread,(tchStatus )tch_mailqInit((tch_mailqId) arg1,(tch_mailqId) arg2));
		break;
	case SV_MAILQ_ALLOC:
		tchk_kernelSetResult(tch_currentThread,tchk_mailqAlloc((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_FREE:
		tchk_kernelSetResult(tch_currentThread,tchk_mailqFree((tch_mailqId) arg1,(void*) arg2));
		break;
	case SV_MAILQ_DEINIT:
		tchk_kernelSetResult(tch_currentThread,tchk_mailqDeinit((tch_mailqId) arg1));
		break;
	case SV_HAL_ENABLE_ISR:
		NVIC_DisableIRQ(arg1);
		NVIC_SetPriority(arg1,arg2);
		NVIC_EnableIRQ(arg1);
		tchk_kernelSetResult(tch_currentThread,tchOK);
		break;
	case SV_HAL_DISABLE_ISR:
		NVIC_DisableIRQ(arg1);
		tchk_kernelSetResult(tch_currentThread,tchOK);
		break;
	}
}

tchStatus tch_kernel_enableInterrupt(IRQn_Type irq,uint32_t priority){
	if(tch_port_isISR())
		return tchErrorISR;
	return tch_port_enterSv(SV_HAL_ENABLE_ISR,irq,priority);
}

tchStatus tch_kernel_disableInterrupt(IRQn_Type irq){
	if(tch_port_isISR())
		return tchErrorISR;
	return tch_port_enterSv(SV_HAL_DISABLE_ISR,irq,0);
}


static DECLARE_THREADROUTINE(systhreadRoutine){
	// perform runtime initialization


	RuntimeInterface.uStdLib = tch_initCrt0(NULL);
	RuntimeInterface.Device = tch_kernelInitHAL(&RuntimeInterface);
	if(!RuntimeInterface.Device)
		tch_kernel_errorHandler(FALSE,tchErrorValue);
	boardHandle = tch_boardInit(&RuntimeInterface);


	RuntimeInterface.Time = tchk_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);

	if(!sysTaskQ)
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	busyMonitor.wrk_load = 0;
	busyMonitor.mtx = env->Mtx->create();


	tch_threadCfg threadcfg;
	Thread->initCfg(&threadcfg);
	threadcfg.t_routine = (tch_thread_routine) main;
	threadcfg.t_priority = Normal;
	threadcfg.t_name = "main";
	threadcfg.t_memDef.heap_sz = 0x800;
	threadcfg.t_memDef.stk_sz = 0x800;

	mainThread = tchk_threadCreateThread(&threadcfg,&RuntimeInterface,TRUE,FALSE);


	if((!mainThread))
		tch_kernel_errorHandler(TRUE,tchErrorOS);


	tch_idleInit();
	Thread->start(mainThread);

	tch_port_enableISR();                   // interrupt enable

	while(TRUE){
		__lwtsk_start_loop();			// start loop lwtask handler
	}
	return tchOK;    // unreachable point
}


