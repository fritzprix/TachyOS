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
#include "tch_board.h"
#include "tch_hal.h"
#include "tch_port.h"

#include "kernel/tch_err.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mailq.h"
#include "kernel/tch_msgq.h"
#include "kernel/tch_thread.h"
#include "kernel/tch_time.h"
#include "kernel/mm/tch_mm.h"


#if !defined(CONFIG_KERNEL_DYNAMICSIZE) || \
	!defined(CONFIG_KERNEL_STACKSIZE) ||\
	!defined(CONFIG_PAGE_SHIFT) ||\
	!defined(CONFIG_KERNEL_STACKLIMIT)
#warning "Kernel is not configured properly"
#endif

#define SYSTSK_ID_SLEEP             ((int) -3)
#define SYSTSK_ID_ERR_HANDLE        ((int) -2)
#define SYSTSK_ID_RESET             ((int) -1)

typedef struct tch_busy_monitor_t {
	tch_mtxId 	mtx;
	uint32_t 	wrk_load;
}tch_busy_monitor;



static DECLARE_THREADROUTINE(systhreadRoutine);

static tch_syscall* __syscall_table = (tch_syscall*) &__syscall_entry;
static tch_busy_monitor busyMonitor;
static tch RuntimeInterface;
static tch_threadId mainThread = NULL;
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

	if(!tch_port_init())										// initialize port layer
		KERNEL_PANIC("tch_sys.c","Port layer is not implmented");


	/*initialize kernel global variable*/
	cdsl_dlistInit((cdsl_dlistNode_t*) &procList);


	__VALID_SYSCALL = FALSE;

	mainThread = NULL;
	sysThread = NULL;

	tch_port_atomicBegin();

	tch_threadCfg thcfg;
	Thread->initCfg(&thcfg, systhreadRoutine, Kernel, 1 << 10, 0, "systhread");
	sysThread = tchk_threadCreateThread(&thcfg,(void*) tch_rti,TRUE,TRUE,NULL);

	tch_schedInit(sysThread);
	return;
}


void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2,uint32_t arg3){
	if(__VALID_SYSCALL)
		__VALID_SYSCALL = FALSE;
	else
		tch_kernel_raise_error(tch_currentThread,tchErrorIllegalAccess,"Illegal System call route detected");


	tch_thread_kheader* cth = NULL;
	tch_thread_kheader* nth = NULL;
	tch_exc_stack* sp = NULL;

	if(sv_id == SV_EXIT_FROM_SV){
		sp = (tch_exc_stack*)tch_port_getUserSP();
			sp++;
			cth = (tch_thread_kheader*) tch_currentThread->kthread;
			cth->tslot = 0;
			cth->state = RUNNING;
			current_mm = &tch_currentThread->kthread->mm;

	#ifdef __NEWLIB__
			_impure_ptr = &tch_currentThread->reent;
	#endif

			tch_port_loadPageTable(tch_currentThread->kthread->mm.pgd);/// apply page mapping
			tch_port_setUserSP((uint32_t)sp);
			if((arg1 = tchk_threadIsValid(tch_currentThread)) == tchOK){
				tch_port_atomicEnd();
			}else{
				tch_schedDestroy(tch_currentThread,arg1);
			}
			if(tchk_threadIsPrivilidged(tch_currentThread))
				tch_port_enablePrivilegedThread();
			else
				tch_port_disablePrivilegedThread();
	}else{
		tchk_kernelSetResult(tch_currentThread,(*((tch_syscall*) ((uint32_t) __syscall_table + sv_id)))(arg1,arg2,arg3));
	}
}


static DECLARE_THREADROUTINE(systhreadRoutine){
	// perform runtime initialization

	RuntimeInterface.uStdLib = tch_initCrt0(NULL);
	tch_port_enableISR();                   // interrupt enable

	RuntimeInterface.Device = tch_hal_init(&RuntimeInterface);
	if(!RuntimeInterface.Device)
		KERNEL_PANIC("tch_sys.c","Hal interface lost");
	boardHandle = tch_boardInit(&RuntimeInterface);


	RuntimeInterface.Time = tchk_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);

	busyMonitor.wrk_load = 0;
	busyMonitor.mtx = env->Mtx->create();


	tch_threadCfg threadcfg;
	Thread->initCfg(&threadcfg,main,Normal,0x800,0x800,"main");
	mainThread = tchk_threadCreateThread(&threadcfg,&RuntimeInterface,TRUE,TRUE,NULL);


	if((!mainThread))
		KERNEL_PANIC("tch_sys.c","Can't create init thread");


	idle_init();
	Thread->start(mainThread);

	while(TRUE){
		__lwtsk_start_loop();			// start loop lwtask handler
	}
	return tchOK;    // unreachable point
}


