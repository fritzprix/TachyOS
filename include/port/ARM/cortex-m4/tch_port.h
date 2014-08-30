/*
 * tch_port.h
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
 */

#ifndef TCHtch_port_H_
#define TCHtch_port_H_


#define GROUP_PRIOR_Pos                (uint8_t) (7)
#define SUB_PRIOR_Pos                  (uint8_t) (4)
#define GROUP_PRIOR(x)                 (uint8_t) ((x & 1) << (GROUP_PRIOR_Pos - SUB_PRIOR_Pos))
#define SUB_PRIOR(y)                   (uint8_t) ((y & 7))

#define MODE_KERNEL                    (uint32_t)(1 << GROUP_PRIOR_Pos)                                 // execution priority of kernel only supervisor call can interrupt
#define MODE_USER                      (uint32_t)(0)


#define HANDLER_SVC_PRIOR              (uint32_t)(GROUP_PRIOR(0) | SUB_PRIOR(0))
#define HANDLER_SYSTICK_PRIOR          (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(1))
#define HANDLER_HIGH_PRIOR             (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(2))
#define HANDLER_NORMAL_PRIOR           (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(3))
#define HANDLER_LOW_PRIOR              (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(4))

#define tch_port_setThreadSP(sp)           __set_PSP(sp)
#define tch_port_getThreadSP()             __get_PSP()
#define tch_port_setHandlerSP(sp)          __set_MSP(sp)
#define tch_port_getHandlerSP()            __get_MSP()

#define FAULT_TYPE_HARD                    (-4)
#define FAULT_TYPE_MEM                     (-3)
#define FAULT_TYPE_BUS                     (-2)
#define FAULT_TYPE_USG                     (-1)



extern void tch_kernel_faulthandle(int faulttype);

extern void tch_port_enableSysTick(void);
extern void tch_port_enableISR(void);
extern void tch_port_disableISR(void);
/***
 *  Kernal lock action
 *   - to guarantee kernel operation not interrupted or preempted
 */
extern void tch_port_kernel_lock(void);
/***
 *  Kernel Unlock action
 *   - to allow interrupt or thread preemption when it's needed
 */
extern void tch_port_kernel_unlock(void);
extern BOOL tch_port_isISR();
extern void tch_port_switchContext(void* nth,void* cth) __attribute__((naked));
extern void tch_port_jmpToKernelModeThread(void* routine,uword_t arg1,uword_t arg2,uword_t retv);
extern int tch_port_enterSvFromUsr(word_t sv_id,uword_t arg1,uword_t arg2);
extern int tch_port_enterSvFromIsr(word_t sv_id,uword_t arg1,uword_t arg2);
extern void* tch_port_makeInitialContext(uaddr_t sp,uaddr_t initfn);
extern int tch_port_exclusiveCompareUpdate(uaddr_t dest,uword_t comp,uword_t update);
extern int tch_port_exclusiveCompareDecrement(uaddr_t dest,uword_t comp);


typedef struct _tch_exc_stack tch_exc_stack;
typedef struct _tch_thread_context tch_thread_context;
typedef struct _arm_sbrtn_ctx arm_sbrtn_ctx;




/**
 * follows arm v7m arch. ref.
 * exception push & pop registers below in the stack at the entry and exit
 */

struct _tch_exc_stack {
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t LR14;
	uint32_t Return;
	uint32_t xPSR;
#if MFEATURE_HFLOAT
	float S0;
	float S1;
	float S2;
	float S3;
	float S4;
	float S5;
	float S6;
	float S7;
	float S8;
	float S9;
	float S10;
	float S11;
	float S12;
	float S13;
	float S14;
	float S15;
	uint32_t FPSCR;
	uint32_t RESV;
#endif
}__attribute__((aligned(8)));

struct _tch_thread_context {
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t LR;
#if MFEATURE_HFLOAT
	float S16;
	float S17;
	float S18;
	float S19;
	float S20;
	float S21;
	float S22;
	float S23;
	float S24;
	float S25;
	float S26;
	float S27;
	float S28;
	float S29;
	float S30;
	float S31;
#endif
}__attribute__((aligned(8)));




#endif /* TCHtch_port_H_ */
