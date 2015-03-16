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
#include "tch_board.h"
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





extern const tch_hal* tch_kernel_initHAL(const tch* ctx);
extern BOOL tch_kernel_initPort();
extern tchStatus tch_kernel_initCrt0(const tch* ctx);


extern tchStatus tch_kernel_postSysTask(int id,tch_sysTaskFn fn,void* arg);
extern tchStatus tch_kernel_enableInterrupt(IRQn_Type irq,uint32_t priority);
extern tchStatus tch_kernel_disableInterrupt(IRQn_Type irq);
extern tchStatus tch_kernel_exec(const void* loadableBin,tch_threadId* nproc);


extern void __tch_kernel_atexit(tch_threadId thread,int res) __attribute__((naked));
extern uint32_t tch_kHeapAvail(void);
extern tchStatus tch_kHeapFreeAll(tch_threadId thread);


/**\!brief Notify kernel that system is busy, so system should be prevented from going into sleep mode
 *
 */
extern void tch_kernelSetBusyMark();

/**\!brief Notify kernel that busy task is finished, so system can be go into sleep mode
 *
 */
extern void tch_kernelClrBusyMark();

/**\!brief check whether system is busy
 *
 */
extern BOOL tch_kernelIsBusy();



extern int Sys_Stack_Top asm("sys_stack_top");
extern int Sys_Stack_Limit asm("sys_stack_limit");

extern int Heap_Base asm("sys_heap_base");
extern int Heap_Limit asm("sys_heap_limit");

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
extern const tch_event_ix* Event;
extern const tch_mailq_ix* MailQ;
extern const tch_mpool_ix* Mempool;
extern const tch_mem_ix* uMem;
extern const tch_mem_ix* kMem;
extern const tch_mem_ix* shMem;
extern const tch_ustdlib_ix* uStdLib;
extern const tch_event_ix* Event;
extern const tch_signal_ix* Sig;

extern const tch_hal* Hal;



extern tch_thread_header* tch_currentThread;
extern volatile uint64_t tch_systimeTick;
extern tch_thread_queue tch_procList;
extern const tch* tch_rti;
extern tch_boardHandle tch_board;
extern tch_memId sharedMem;



#endif /* TCH_KERNEL_H_ */
