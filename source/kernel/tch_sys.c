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

#include "kernel/tch_waitq.h"
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
#include "kernel/tch_dbg.h"
#include "kernel/tch_kmod.h"
#include "kernel/mm/tch_mm.h"
#include "kernel/mm/tch_malloc.h"



#if !defined(KERNEL_DYNAMIC_SIZE) || \
	!defined(KERNEL_STACK_SIZE) ||\
	!defined(PAGE_OFFSET) ||\
	!defined(KERNEL_STACK_LIMIT)
#warning "Kernel is not configured properly"
#endif


static DECLARE_THREADROUTINE(systhreadRoutine);


static tch_syscall* __syscall_table = (tch_syscall*) &__syscall_entry;
__USER_RODATA__ const tch_core_api_t RuntimeInterface = {
		.Thread = &Thread_IX,
		.Mtx = &Mutex_IX,
		.Sem = &Semaphore_IX,
		.Time = &Time_IX,
		.Condv = &CondVar_IX,
		.Barrier = &Barrier_IX,
		.WaitQ = &WaitQ_IX,
		.Mempool = &MPool_IX,
		.MailQ = &MailQ_IX,
		.MsgQ = &MsgQ_IX,
		.Mem = &UMem_IX,
		.Event = &Event_IX,
		.Module = &Module_IX,
		.Dbg = &Debug_IX
};

const tch_core_api_t* tch_rti = &RuntimeInterface;

static tch_threadId sysThread;

volatile BOOL kernel_ready;
volatile BOOL __VALID_SYSCALL;
tch_board_descriptor board_desc;


/***
 *  Kernel initailization is followed sequence below
 *  1. initialize kernel stack and dyanmic memory region @ tch_kernelMemInit()
 *  2. perform cpu specific initialization
 */
void tch_kernel_init(void* arg){

	kernel_ready = FALSE;

	// initialize port layer
	tch_port_init();
	board_desc = tch_board_init(tch_rti);

	/*initialize kernel global variable*/
	__VALID_SYSCALL = FALSE;
	sysThread = NULL;

	tch_port_atomicBegin();

	thread_config_t thcfg;
	Thread->initConfig(&thcfg, systhreadRoutine, Kernel, 1 << 10, 0, "systhread");
	sysThread = tch_thread_createThread(&thcfg,(void*) tch_rti,TRUE,TRUE,NULL);
	tch_schedInit(sysThread);
}


void tch_kernel_onSyscall(uint32_t sv_id,uint32_t arg1, uint32_t arg2,uint32_t arg3){
	if(__VALID_SYSCALL)
		__VALID_SYSCALL = FALSE;
	else
		tch_kernel_onSoftException(current,tchErrorIllegalAccess,"Illegal System call detected");


	tch_thread_kheader* cth = NULL;
	tch_thread_kheader* nth = NULL;
	tch_exc_stack* sp = NULL;

	if(sv_id == SV_EXIT_FROM_SWITCH){
		sp = (tch_exc_stack*) tch_port_getUserSP();
		sp++;
		cth = (tch_thread_kheader*) current->kthread;
		cth->tslot = 0;
		cth->state = RUNNING;
		current_mm = &current->kthread->mm;

		tch_port_loadPageTable(current->kthread->mm.pgd);/// apply page mapping

		tch_port_setUserSP((uwaddr_t) sp);
		tch_port_atomicEnd();

		if ((tch_thread_isValid(current) != tchOK) || !tch_thread_isLive(current))
		{
			tch_thread_exit(current, current->kRet);
		}
		if (tch_thread_isPrivilidged(current))
			tch_port_enablePrivilegedThread();
		else
			tch_port_disablePrivilegedThread();
	}else{
		tch_kernel_set_result(current,(*((tch_syscall*) ((uint32_t) __syscall_table + sv_id)))(arg1,arg2,arg3));
	}
}


static DECLARE_THREADROUTINE(systhreadRoutine){

	/** perform runtime initialization **/
	tch_kmod_init();
	tch_port_enableGlobalInterrupt();                   // interrupt enable
	idle_init();
	kernel_ready = TRUE;

	__lwtsk_init();
	tch_dbg_init(board_desc->b_logfile);

	print_dbg("=== TachyOS Kernel Start === \n\r");

	tch_systimeInit(&RuntimeInterface,__BUILD_TIME_EPOCH,UTC_P9);

	struct tm localtime;
	tch_rti->Time->fromEpochTime(__BUILD_TIME_EPOCH, &localtime, UTC_P9);


	print_dbg("- Timer initialized\n\r");
	print_dbg("  -> Current Local Date %d / %d / %d\n\r",1900 + localtime.tm_year, localtime.tm_mon + 1, localtime.tm_mday);
	print_dbg("                   Time %d : %d : %d\n\r",localtime.tm_hour,localtime.tm_min, localtime.tm_sec);


	while(TRUE)
	{
		__lwtsk_start_loop();			// start loop lwtask handler
	}

	tch_kmod_exit();
	return tchOK;    					// unreachable point
}


