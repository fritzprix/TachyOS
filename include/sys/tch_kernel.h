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
#include "tch_ktypes.h"
#include "tch_port.h"
#include "tch_hal.h"
#include "tch_mem.h"
#include "tch_list.h"

#define tch_kernelSetResult(th,result) ((tch_thread_header*) th)->t_kRet = result
#define tch_kAssert(b,error)  if(!b){__tch_printError(error);Thread->terminate(tch_currentThread,error);}

/*!
 * \brief
 */
extern void tch_kernelInit(void* arg);
extern void tch_kernelSysTick(void);
extern void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2);
extern BOOL tch_kernelThreadIntegrityCheck(tch_threadId thrtochk);
extern uint64_t tch_kernelCurrentSystick();
extern const tch_hal* tch_kernel_initHAL();
extern BOOL tch_kernel_initPort();


extern int Sys_Stack_Top asm("sys_stack_top");
extern int Heap_Base asm("heap_base");
extern int Heap_Limit asm("heap_limit");

extern int Main_Stack_Top asm("main_stack_top");
extern int Main_Stack_Limit asm("main_stack_limit");

extern int Idle_Stack_Top asm("idle_stack_top");
extern int Idle_Stack_Limit asm("idle_stack_limit");


extern tch_thread_header* tch_currentThread;
#ifndef __USE_MALLOC
extern tch_mem_handle* Heap_Manager;
#endif



void tch_kernel_errorHandler(BOOL dump,tchStatus status) __attribute__((naked));


/**
 * Kernel API Struct* List
 * - are bound statically in compile time
 */
extern const tch_thread_ix* Thread;
extern const tch_signal_ix* Sig;
extern const tch_timer_ix* Timer;
extern const tch_condv_ix* Condv;
extern const tch_mtx_ix* Mtx;
extern const tch_semaph_ix* Sem;
extern const tch_bar_ix* Barrier;
extern const tch_msgq_ix* MsgQ;
extern const tch_mailq_ix* MailQ;
extern const tch_mpool_ix* Mempool;
extern const tch_mem_ix* Mem;
extern const tch_hal* Hal;

extern const tch_kernel_instance* Sys;



#endif /* TCH_KERNEL_H_ */
