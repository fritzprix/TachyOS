/*
 * tch_kernel.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 22.
 *      Author: innocentevil
 */

#ifndef TCH_KERNEL_H_
#define TCH_KERNEL_H_

/***
 *   Kernel Interface
 *   - All kernel functions(like scheduling,synchronization) are based on this module
 *   - Initialize kernel enviroment (init kernel internal objects,
 */
#include "tch.h"
#include "tch_kcfg.h"
#include "tch_ktypes.h"
#include "tch_port.h"
#include "tch_sched.h"
#include "tch_hal.h"




#if !defined(__BUILD_TIME_EPOCH)
#define __BUILD_TIME_EPOCH 0UL
#endif

#define TCH_SYS_TASKQ_SZ                     (16)
#define DECLARE_SYSTASK(fn)                  void fn(int id,const tch* env,void* arg)
#define tch_kernelSetResult(th,result) ((tch_thread_header*) th)->t_kRet = result
#define getThreadHeader(th_id)  ((tch_thread_header*) th_id)


/*!
 * \brief
 */
extern void tch_kernelInit(void* arg);
extern void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2);
extern void tch_KernelOnSystick();
extern void tch_kernelOnWakeup();
extern void tch_kernelOnHardFault(int fault,int type);




extern const tch_hal* tch_kernel_initHAL(const tch* env);
extern BOOL tch_kernel_initPort();
extern tchStatus tch_kernel_initCrt0(tch* env);
extern tchStatus tch_kernel_postSysTask(int id,tch_sysTaskFn fn,void* arg);
extern void __tch_kernel_atexit(tch_threadId thread,int res) __attribute__((naked));
extern uint32_t tch_kHeapAvail(void);
extern tchStatus tch_kHeapFreeAll(tch_threadId thread);




extern int Sys_Stack_Top asm("sys_stack_top");
extern int Sys_Stack_Limit asm("sys_stack_limit");

extern int Heap_Base asm("heap_base");
extern int Heap_Limit asm("heap_limit");

extern int Main_Stack_Top asm("main_stack_top");
extern int Main_Stack_Limit asm("main_stack_limit");

extern int Idle_Stack_Top asm("idle_stack_top");
extern int Idle_Stack_Limit asm("idle_stack_limit");

void tch_kernel_errorHandler(BOOL dump,tchStatus status) __attribute__((naked));


/**
 * Kernel API Struct* List
 * - are bound statically in compile time
 */
extern const tch_thread_ix* Thread;
extern const tch_condv_ix* Condv;
extern const tch_mtx_ix* Mtx;
extern const tch_semaph_ix* Sem;
extern const tch_bar_ix* Barrier;
extern const tch_msgq_ix* MsgQ;
extern const tch_mailq_ix* MailQ;
extern const tch_mpool_ix* Mempool;
extern const tch_mem_ix* uMem;
extern const tch_mem_ix* kMem;
extern const tch_mem_ix* shMem;
extern const tch_ustdlib_ix* uStdLib;
extern const tch_event_ix* Event;

extern const tch_hal* Hal;



extern tch_thread_header* tch_currentThread;
extern volatile uint64_t tch_systimeTick;
extern tch_thread_queue tch_procList;
extern const tch* tch_rti;
extern tch_memId sharedMem;



#endif /* TCH_KERNEL_H_ */
