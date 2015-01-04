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
#include "tch_syscfg.h"
#include "tch_thread.h"
#include "tch_time.h"
#include "tch_board.h"



#define SYSTSK_ID_SLEEP             ((int) -3)
#define SYSTSK_ID_ERR_HANDLE        ((int) -2)
#define SYSTSK_ID_RESET             ((int) -1)


static DECLARE_THREADROUTINE(systhreadRoutine);
static DECLARE_THREADROUTINE(idle);
static DECLARE_SYSTASK(kernelTaskHandler);



static tch RuntimeInterface;
const tch* tch_rti = &RuntimeInterface;

static tch_threadId mainThread = NULL;
static tch_threadId idleThread = NULL;

static tch_thread_queue tch_lpTickWaitQ;
static tch_thread_queue tch_lpEvtWaitQ;

static tch_mailqId sysTaskQ;
tch_memId sharedMem;
const struct tch_bin_descriptor BIN_DESC;
BOOL lp_handled;


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
	RuntimeInterface.Mempool = Mempool;
	RuntimeInterface.MailQ = MailQ;
	RuntimeInterface.MsgQ = MsgQ;
	RuntimeInterface.Mem = uMem;


	tch_listInit((tch_lnode_t*) &tch_procList);
	tch_listInit((tch_lnode_t*) &tch_lpTickWaitQ);
	tch_listInit((tch_lnode_t*) &tch_lpEvtWaitQ);

	uint8_t* shMemBlk = kMem->alloc(TCH_CFG_SHARED_MEM_SIZE);
	sharedMem = tch_memCreate(shMemBlk,TCH_CFG_SHARED_MEM_SIZE);

	/**
	 *  dynamic binding of dependecy
	 */

	if(!tch_kernel_initPort()){
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	}

	tch_port_kernel_lock();

	tch_threadCfg thcfg;
	thcfg._t_name = "sysloop";
	thcfg._t_routine = systhreadRoutine;
	thcfg.t_proior = KThread;
	thcfg.t_stackSize = 1 << 10;
	tch_currentThread = ROOT_THREAD;
	sysThread = Thread->create(&thcfg,(void*)tch_rti);

	tch_port_enableISR();                   // interrupt enable
	tch_schedInit(sysThread);
	return;
}



void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_thread_header* cth = NULL;
	tch_thread_header* nth = NULL;
	tch_exc_stack* sp = NULL;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp = (tch_exc_stack*)tch_port_getThreadSP();
		sp++;
		tch_currentThread->t_tslot = 0;
		_impure_ptr = &tch_currentThread->t_reent;
		tch_port_setThreadSP((uint32_t)sp);
		if(tch_schedLivenessChk(tch_currentThread))
			tch_port_kernel_unlock();
		break;
	case SV_THREAD_START:              // start thread first time
		tch_schedStartThread((tch_threadId) arg1);
		break;
	case SV_THREAD_YIELD:
		tch_schedSleepThread(arg1,WAIT);    // put current thread in pending queue and will be waken up at given after given time duration is passed
		break;
	case SV_THREAD_SLEEP:
		cth = tch_currentThread;
		tch_schedSuspendThread(&tch_lpTickWaitQ,osWaitForever);
		cth->t_state = SLEEP;
		break;
	case SV_THREAD_JOIN:
		if(((tch_thread_header*)arg1)->t_state != TERMINATED){                                 // check target if thread has terminated
			tch_schedSuspendThread((tch_thread_queue*)&((tch_thread_header*)arg1)->t_joinQ,arg2);    //if not, thread wait
			break;
		}
		tch_kernelSetResult(tch_currentThread,tchOK);                                           //..otherwise, it returns immediately
		break;
	case SV_THREAD_RESUME:
		tch_schedResumeM((tch_thread_queue*) arg1,1,arg2,TRUE);
		break;
	case SV_THREAD_RESUMEALL:
		tch_schedResumeM((tch_thread_queue*) arg1,SCHED_THREAD_ALL,arg2,TRUE);
		break;
	case SV_THREAD_SUSPEND:
		tch_schedSuspendThread((tch_thread_queue*)arg1,arg2);
		break;
	case SV_THREAD_TERMINATE:
		cth = (tch_thread_header*) arg1;
		tch_schedTerminateThread((tch_threadId) cth,arg2);
		break;
	case SV_THREAD_DESTROY:
		cth = (tch_thread_header*) arg1;
		tch_schedDestroyThread((tch_threadId) cth,arg2);
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
	tchStatus result = tchOK;
	tch_sysTask* task = tch_rti->MailQ->alloc(sysTaskQ,osWaitForever,&result);
	task->arg = arg;
	task->fn = fn;
	task->id = id;
	task->prior = tch_currentThread->t_prior;
	task->status = tchOK;
	if(result == tchOK)
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

	tchEvent evt;
	tch_sysTask* task = NULL;

	RuntimeInterface.Device = tch_kernel_initHAL(&RuntimeInterface);
	if(!RuntimeInterface.Device)
		tch_kernel_errorHandler(FALSE,tchErrorValue);

	RuntimeInterface.Time = tch_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9,tch_kernelOnWakeup);

	if(tch_kernel_initCrt0(&RuntimeInterface) != tchOK)
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	sysTaskQ = MailQ->create(sizeof(tch_sysTask),TCH_SYS_TASKQ_SZ);
	if(!sysTaskQ)
		tch_kernel_errorHandler(TRUE,tchErrorOS);

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
	idleThread = Thread->create(&thcfg,NULL);


	if((!mainThread) || (!idleThread))
		tch_kernel_errorHandler(TRUE,tchErrorOS);

	tch_boardInit(&RuntimeInterface);


	Thread->start(idleThread);
	Thread->start(mainThread);

	uStdLib->string->memset(&evt,0,sizeof(tchEvent));



	// loop for handling system tasks (from ISR / from any user thread)
	while(TRUE){
		evt = MailQ->get(sysTaskQ,osWaitForever);
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
		if((tch_currentThread->t_tslot > 10) && tch_schedIsEmpty()  && tch_systimeHasPending())
			tch_kernel_postSysTask(SYSTSK_ID_SLEEP,kernelTaskHandler,rtc_handle);
		tch_hal_enterSleepMode();
		// some function waking up from sleep mode
	}
	return 0;
}


void tch_kernelOnWakeup(){
	tch_schedResumeM(&tch_lpTickWaitQ,SCHED_THREAD_ALL,tchOK,TRUE);
}


static DECLARE_SYSTASK(kernelTaskHandler){
	struct tch_err_descriptor* err = NULL;
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
		env->uStdLib->stdio->iprintf("\r\n===SYSTEM FAULT (SUBJECT : %s) (ERR TYPE : %d)===\n",getThreadHeader(err->subj)->t_name,err->errtype);
		env->uStdLib->stdio->iprintf("\r\n===Local Time : %d/%d/%d %d:%d:%d ===============\n",lt.tm_year + 1900,lt.tm_mon + 1,lt.tm_mday,lt.tm_hour,lt.tm_min,lt.tm_sec);
		env->Thread->terminate(err->subj,err->errno);
		kMem->free(err);
		break;
	case SYSTSK_ID_RESET:
		tch_boardDeinit(env);
		tch_port_reset();
		break;
	}
}


void tch_kernelOnHardFault(int fault,int type){
	struct tch_err_descriptor* err = kMem->alloc(sizeof(struct tch_err_descriptor));
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
	while(1){
		asm volatile("NOP");
	}
}



