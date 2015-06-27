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
static DECLARE_THREADROUTINE(idle);
static DECLARE_SYSTASK(kernelTaskHandler);


static tch_busy_monitor busyMonitor;
static tch RuntimeInterface;
static tch_threadId mainThread = NULL;
static tch_threadId idleThread = NULL;
static tch_mailqId sysTaskQ;
static tch_threadId sysThread;

tch_thread_queue procList;
tch_boardHandle boardHandle = NULL;
BOOL __VALID_SYSCALL;

const tch* tch_rti = &RuntimeInterface;


/***
 *  Kernel initailization is followed sequence below
 *  1. initialize kernel stack and dyanmic memory region @ tch_kernelMemInit()
 *  2. perform cpu specific initialization
 */
void tch_kernelInit(void* arg){

	kernel_descriptor.k_stacktop = tch_kernelMemInit(NULL);

	if(!tch_kernelInitPort(&kernel_descriptor))										// initialize port layer
		tch_kernel_errorHandler(FALSE,tchErrorOS);


	/*initialize kernel global variable*/
	cdsl_dlistInit((cdsl_dlistNode_t*) &procList);


	__VALID_SYSCALL = FALSE;
	/*Bind API Object*/


	RuntimeInterface.uStdLib = tch_initCrt0();

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
	idleThread = NULL;
	sysThread = NULL;

	/**
	 *  Initialize pageing sub-system for memory managment
	 */
	if(tchk_pageInit(&Sys_Heap_Base,((uword_t)&Sys_Heap_Limit - (uword_t)&Sys_Heap_Base)) != tchOK)
		tch_kernel_errorHandler(FALSE,tchErrorOS);

	tchk_shareableMemInit(TCH_CFG_SHARED_MEM_SIZE);					// Initialize shareable(publicly accessable from all execution context) memory allocator
	tchk_kernelHeapInit(TCH_CFG_KERNEL_HEAP_MEM_SIZE);				// Initialize kernel heap allocator(only accessible from privilidged level)

	tch_port_kernel_lock();

	tch_threadCfg thcfg;
	Thread->initCfg(&thcfg);
	thcfg.t_name = "systhread";
	thcfg.t_routine = systhreadRoutine;
	thcfg.t_priority = Kernel;
	thcfg.t_memDef.stk_sz = 1 << 10;
	sysThread = tchk_threadCreateThread(&thcfg,(void*) tch_rti,TRUE,TRUE);

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(sysThread);
	return;
}


tchStatus tch_kernel_exec(const void* loadableBin,tch_threadId* nproc){
	if(!loadableBin)
		return tchErrorParameter;

	tch_dynamic_bin_header header = (tch_dynamic_bin_header) loadableBin;
//	loadableBin += sizeof(struct tch_dynamic_bin_meta_struct);
	uint8_t* exImg = (uint8_t*) tchk_kernelHeapAlloc(header->b_sz);
	uStdLib->string->memcpy(exImg,loadableBin,header->b_sz);
	tch_thread_routine entry = (tch_thread_routine)(((uint32_t)exImg + header->b_entry) | 0x1); // the address value for indirect branch in ARM should be 1 in its '0' bit, otherwise usagefault
	tch_threadCfg thcfg;
	thcfg.t_memDef.heap_sz = 0;
	thcfg.t_memDef.stk_sz = (1 << 9);
	thcfg.t_name = "Test";
	thcfg.t_routine = entry;
	thcfg.t_priority = Normal;

	*nproc = tchk_threadCreateThread(&thcfg,NULL,TRUE,FALSE);
	return tchOK;
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
		sp = (tch_exc_stack*)tch_port_getThreadSP();
		sp++;
		cth = (tch_thread_kheader*) tch_currentThread->t_kthread;
		cth->t_tslot = 0;
		cth->t_state = RUNNING;

#ifdef __NEWLIB__
		_impure_ptr = &tch_currentThread->t_reent;
#endif

		tchk_mapPage(cth->t_pgId);			/// apply page mapping
		tch_port_setThreadSP((uint32_t)sp);
		if((arg1 = tchk_threadIsValid(tch_currentThread)) == tchOK){
			tch_port_kernel_unlock();
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

tchStatus tch_kernel_postSysTask(int id,tch_sysTaskFn fn,void* arg){
	tchStatus result = tchOK;
	tch_sysTask* task = tch_rti->MailQ->alloc(sysTaskQ,tchWaitForever,&result);
	task->arg = arg;
	task->fn = fn;
	task->id = id;
	task->prior = getThreadKHeader(tch_currentThread)->t_prior;
	task->status = tchOK;
	if(result == tchOK)
		tch_rti->MailQ->put(sysTaskQ,task);
	return result;
}


void tch_kernelSetBusyMark(){
	if(RuntimeInterface.Mtx->lock(busyMonitor.mtx,tchWaitForever) != tchOK)
		return;
	busyMonitor.wrk_load++;
	RuntimeInterface.Mtx->unlock(busyMonitor.mtx);
}

void tch_kernelClrBusyMark(){
	if(RuntimeInterface.Mtx->lock(busyMonitor.mtx,tchWaitForever) != tchOK)
		return;
	busyMonitor.wrk_load--;
	RuntimeInterface.Mtx->unlock(busyMonitor.mtx);
}

BOOL tch_kernelIsBusy(){
	return !busyMonitor.wrk_load;
}

void tch_kernel_errorHandler(BOOL dump,tchStatus status){

	/**
	 *  optional register dump
	 */
	while(1){
		asm volatile("NOP");
	}
}

static DECLARE_THREADROUTINE(systhreadRoutine){
	// perform runtime initialization

	tchEvent evt;
	tch_sysTask* task = NULL;

	RuntimeInterface.Device = tch_kernelInitHAL(&RuntimeInterface);
	if(!RuntimeInterface.Device)
		tch_kernel_errorHandler(FALSE,tchErrorValue);
	boardHandle = tch_boardInit(&RuntimeInterface);


	RuntimeInterface.Time = tchk_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);

	sysTaskQ = MailQ->create(sizeof(tch_sysTask),TCH_SYS_TASKQ_SZ);
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

	threadcfg.t_routine = (tch_thread_routine) idle;
	threadcfg.t_priority = Idle;
	threadcfg.t_name = "idle";
	threadcfg.t_memDef.heap_sz = 0x200;
	threadcfg.t_memDef.stk_sz = TCH_CFG_THREAD_STACK_MIN_SIZE;
	idleThread = Thread->create(&threadcfg,NULL);


	if((!mainThread) || (!idleThread))
		tch_kernel_errorHandler(TRUE,tchErrorOS);


	Thread->start(idleThread);
	Thread->start(mainThread);

	uStdLib->string->memset(&evt,0,sizeof(tchEvent));


	// loop for handling system tasks (from ISR / from any user thread)
	while(TRUE){
		evt = MailQ->get(sysTaskQ,tchWaitForever);
		if(evt.status == tchEventMail){
			task = (tch_sysTask*) evt.value.p;
			task->fn(task->id,&RuntimeInterface,task->arg);
			MailQ->free(sysTaskQ,evt.value.p);
		}
	}
	return tchOK;    // unreachable point
}


static DECLARE_THREADROUTINE(idle){
	tch_rtcHandle* rtc_handle = env->Thread->getArg();

	while(TRUE){
		// some function entering sleep mode
		if((!busyMonitor.wrk_load) && (getThreadKHeader(tch_currentThread)->t_tslot > 5) && tch_schedIsEmpty()  && tch_systimeIsPendingEmpty())
			tch_kernel_postSysTask(SYSTSK_ID_SLEEP,kernelTaskHandler,rtc_handle);
		tch_hal_enterSleepMode();
		// some function waking up from sleep mode
 	}
	return 0;
}


static DECLARE_SYSTASK(kernelTaskHandler){
	tch_errorDescriptor* err = NULL;
	struct tm lt;
	switch(id){
	case SYSTSK_ID_SLEEP:
		tch_hal_disableSystick();
		tch_hal_setSleepMode(LP_LEVEL1);
		tch_hal_suspendSysClock();
		tch_hal_enterSleepMode();
		tch_hal_resumeSysClock();
		tch_hal_setSleepMode(LP_LEVEL0);
		tch_hal_enableSystick();
		break;
	case SYSTSK_ID_ERR_HANDLE:
		// handle err
		err = arg;
		env->Time->getLocaltime(&lt);
		env->Thread->terminate(err->subj,err->errno);
		tchk_kernelHeapFree(err);
		break;
	case SYSTSK_ID_RESET:
		tch_boardDeinit(env);
		tch_port_reset();
		break;
	}
}


void tch_kernelOnHardFault(int fault,int type){
	tch_errorDescriptor* err = (tch_errorDescriptor*) tchk_kernelHeapAlloc(sizeof(tch_errorDescriptor));
	err->errtype = fault;
	err->errno = type;
	err->subj = tch_currentThread;
	switch(fault){
	case HARDFAULT_UNRECOVERABLE:
		tch_kernel_postSysTask(SYSTSK_ID_RESET,kernelTaskHandler,err);
		tch_port_clearFault(type);
		return;
	case HARDFAULT_RECOVERABLE:
		tch_kernel_postSysTask(SYSTSK_ID_ERR_HANDLE,kernelTaskHandler,err);
		return;
	}
	while(TRUE){
		asm volatile("NOP");
	}
}



