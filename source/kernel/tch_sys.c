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

#include "kernel/tch_thread.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_bar.h"
#include "kernel/tch_sem.h"
#include "kernel/tch_mpool.h"
#include "kernel/tch_event.h"
#include "kernel/tch_err.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mailq.h"
#include "kernel/tch_msgq.h"
#include "kernel/tch_thread.h"
#include "kernel/tch_time.h"
#include "kernel/tch_kmod.h"
#include "kernel/mm/tch_mm.h"
#include "kernel/mm/tch_malloc.h"



#if !defined(CONFIG_KERNEL_DYNAMICSIZE) || \
	!defined(CONFIG_KERNEL_STACKSIZE) ||\
	!defined(CONFIG_PAGE_SHIFT) ||\
	!defined(CONFIG_KERNEL_STACKLIMIT)
#warning "Kernel is not configured properly"
#endif


static DECLARE_THREADROUTINE(systhreadRoutine);


static tch_syscall* __syscall_table = (tch_syscall*) &__syscall_entry;
static tch RuntimeInterface = {
		.Thread = &Thread_IX,
		.Mtx = &Mutex_IX,
		.Sem = &Semaphore_IX,
		.Condv = &CondVar_IX,
		.Barrier = &Barrier_IX,
		.Mempool = &MPool_IX,
		.MailQ = &MailQ_IX,
		.MsgQ = &MsgQ_IX,
		.Mem = &UMem_IX,
		.Event = &Event_IX,
		.Service = &Service_IX
};
__USER_RODATA__ const tch* tch_rti = &RuntimeInterface;

static tch_threadId sysThread;
volatile BOOL kernel_ready;
BOOL __VALID_SYSCALL;


/***
 *  Kernel initailization is followed sequence below
 *  1. initialize kernel stack and dyanmic memory region @ tch_kernelMemInit()
 *  2. perform cpu specific initialization
 */
void tch_kernelInit(void* arg){

	kernel_ready = FALSE;

	if(!tch_port_init())										// initialize port layer
		KERNEL_PANIC("tch_sys.c","Port layer is not implmented");

	/*initialize kernel global variable*/
	__VALID_SYSCALL = FALSE;
	sysThread = NULL;

	tch_port_atomicBegin();

	thread_config_t thcfg;
	Thread->initConfig(&thcfg, systhreadRoutine, Kernel, 1 << 10, 0, "systhread");
	sysThread = tch_threadCreateThread(&thcfg,(void*) tch_rti,TRUE,TRUE,NULL);
	tch_schedInit(sysThread);

	return;
}


void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2,uint32_t arg3){
	if(__VALID_SYSCALL)
		__VALID_SYSCALL = FALSE;
	else
		tch_kernel_raiseError(current,tchErrorIllegalAccess,"Illegal System call route detected");


	tch_thread_kheader* cth = NULL;
	tch_thread_kheader* nth = NULL;
	tch_exc_stack* sp = NULL;

	if(sv_id == SV_EXIT_FROM_SV){
		sp = (tch_exc_stack*)tch_port_getUserSP();
			sp++;
			cth = (tch_thread_kheader*) current->kthread;
			cth->tslot = 0;
			cth->state = RUNNING;
			current_mm = &current->kthread->mm;

			tch_port_loadPageTable(current->kthread->mm.pgd);/// apply page mapping
			tch_port_setUserSP((uint32_t)sp);
			if((arg1 = tch_threadIsValid(current)) == tchOK){
				tch_port_atomicEnd();
			}else{
				tch_thread_exit(current,arg1);
			}
			if(tch_threadIsPrivilidged(current))
				tch_port_enablePrivilegedThread();
			else
				tch_port_disablePrivilegedThread();
	}else{
		tchk_kernelSetResult(current,(*((tch_syscall*) ((uint32_t) __syscall_table + sv_id)))(arg1,arg2,arg3));
	}
}


static DECLARE_THREADROUTINE(systhreadRoutine){

	/** perform runtime initialization **/
	tch_kmod_init();
	tch_port_enableISR();                   // interrupt enable
	kernel_ready = TRUE;




	RuntimeInterface.Time = tchk_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);
	idle_init();

	while(TRUE){
		__lwtsk_start_loop();			// start loop lwtask handler
	}

	tch_kmod_exit();
	return tchOK;    					// unreachable point
}


