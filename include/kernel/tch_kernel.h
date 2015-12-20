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
#include "autogen.h"
#include "tch_kobj.h"
#include "tch_board.h"
#include "tch_ktypes.h"
#include "tch_port.h"
#include "tch_sched.h"
#include "tch_hal.h"
#include "tch_mm.h"



#if !defined(__BUILD_TIME_EPOCH)
#define __BUILD_TIME_EPOCH 0UL
#endif

#ifndef offsetof
#define offsetof(type,member)					(size_t)(&((type*) 0)->member)
#endif


#ifndef container_of
#define container_of(ptr,type,member) 		(type*) (((size_t) ptr - (size_t) offsetof(type,member)))
#endif


#define __SYSCALL__	__attribute__((section(".sysc.table")))



#define __UACESS					///< used to inform the variable or parameter assigned must be user accessible

#define __USER_API__	__attribute__((section(".utext")))
#define __USER_RODATA__	__attribute__((section(".urodata")))
#define __USER_DATA		__attribute__((section(".udata")))


typedef uword_t (*tch_syscall)(uint32_t arg1,uint32_t arg2,uint32_t arg3);
extern uint32_t __syscall_entry;

#define DECLARE_SYSCALL_3(fn,t1,t2,t3,rt) \
	static rt __##fn(t1,t2,t3)

#define DEFINE_SYSCALL_3(fn,t1,p1,t2,p2,t3,p3,rt)	\
	const __SYSCALL__ void* __entry__##fn = __##fn;\
	static rt __##fn(t1 p1,t2 p2, t3 p3)

#define __SYSCALL_3(fn,arg1,arg2,arg3) \
	tch_port_enterSv((uint32_t) &__entry__##fn - (uint32_t) &__syscall_entry,(uword_t) arg1,(uword_t) arg2,(uword_t) arg3)


#define DECLARE_SYSCALL_2(fn,t1,t2,rt) \
	static rt __##fn(t1, t2)

#define DEFINE_SYSCALL_2(fn,t1,p1,t2,p2,rt)  \
	const __SYSCALL__ void* __entry__##fn = __##fn;\
	static rt __##fn(t1 p1,t2 p2)

#define __SYSCALL_2(fn,arg1,arg2) \
	tch_port_enterSv((uint32_t) &__entry__##fn - (uint32_t) &__syscall_entry,(uword_t) arg1, (uword_t) arg2,0)


#define DECLARE_SYSCALL_1(fn,t1,rt) \
	static rt __##fn(t1)

#define DEFINE_SYSCALL_1(fn,t1,p1,rt) \
	const __SYSCALL__ void* __entry__##fn = __##fn;\
	static rt __##fn(t1 p1)

#define __SYSCALL_1(fn,arg1)\
	tch_port_enterSv((uint32_t) &__entry__##fn - (uint32_t) &__syscall_entry,(uword_t) arg1,0,0)


#define DECLARE_SYSCALL_0(fn,rt) \
	static rt __##fn(void)

#define DEFINE_SYSCALL_0(fn,rt) \
	const __SYSCALL__ void* __entry__##fn = __##fn;\
	static rt __##fn(void)

#define __SYSCALL_0(fn)\
	tch_port_enterSv((uint32_t) &__entry__##fn - (uint32_t) &__syscall_entry,0,0,0)





#define TCH_SYS_TASKQ_SZ                    (16)
#define tchk_kernelSetResult(th,result)		((tch_thread_uheader*) th)->kRet = (uint32_t) result
#define getThreadHeader(th_id)  			((tch_thread_uheader*) th_id)
#define getThreadKHeader(th_id) 			(((tch_thread_uheader*) th_id)->kthread)

/*!
 * \brief
 */
extern __attribute__((naked)) void __init(void* arg);
extern void tch_kernelInit(void* arg);
extern void tch_kernelOnSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2,uint32_t arg3);
extern void tch_KernelOnSystick();
extern void tch_kernelOnWakeup();

extern void tch_kernelOnMemFault(paddr_t pa, int fault);
extern void tch_kernel_handleHardFault(int fault);




extern tchStatus __tch_noop_destr(tch_kobj* obj);
extern void __tch_thread_atexit(tch_threadId thread,int res) __attribute__((naked));


/**\!brief Notify kernel that system is busy, so system should be prevented from going into sleep mode
 *
 */
extern void idle_set_busy();

/**\!brief Notify kernel that busy task is finished, so system can be go into sleep mode
 *
 */
extern void idle_clear_busy();

/**\!brief check whether system is busy
 *
 */
extern BOOL idle_is_busy();


/**
 * Kernel API Struct* List
 * - are bound statically in compile time
 */
extern const tch_kernel_service_thread* Thread;
extern const tch_kernel_service_condv* Condv;
extern const tch_kernel_service_mtx* Mtx;
extern const tch_kernel_service_semaphore* Sem;
extern const tch_kernel_service_barrier* Barrier;
extern const tch_kernel_service_messageQ* MsgQ;
extern const tch_kernel_service_event* Event;
extern const tch_kernel_service_mailQ* MailQ;
extern const tch_kernel_service_mempool* Mempool;
extern const tch_kernel_service_mem* uMem;
extern const tch_kernel_service_svcmanager* Service;
extern const tch_kernel_service_time* Time;





extern const tch_kernel_descriptor kernel_descriptor;
extern const tch* tch_rti;
extern tch_thread_queue procList;
extern tch_thread_uheader* current;
extern volatile BOOL __VALID_SYSCALL;
extern volatile BOOL kernel_ready;
extern volatile uint64_t systimeTick;




#endif /* TCH_KERNEL_H_ */
