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


#include "tch.h"
#include "tch_portcfg.h"



#define tch_port_setThreadSP(sp)           __set_PSP(sp)
#define tch_port_getThreadSP()             __get_PSP()
#define tch_port_setHandlerSP(sp)          __set_MSP(sp)
#define tch_port_getHandlerSP()            __get_MSP()


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
extern void tch_port_jmpToKernelModeThread(void* routine,uint32_t arg1,uint32_t arg2,uint32_t retv);
extern int tch_port_enterSvFromUsr(int sv_id,uint32_t arg1,uint32_t arg2);
extern int tch_port_enterSvFromIsr(int sv_id,uint32_t arg1,uint32_t arg2);
extern void* tch_port_makeInitialContext(void* sp,void* initfn);


typedef struct _tch_exc_stack tch_exc_stack;
typedef struct _tch_thread_context tch_thread_context;

BOOL tch_port_init();


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
#ifdef FEATURE_HFLOAT
	uint32_t S0;
	uint32_t S1;
	uint32_t S2;
	uint32_t S3;
	uint32_t S4;
	uint32_t S5;
	uint32_t S6;
	uint32_t S7;
	uint32_t S8;
	uint32_t S9;
	uint32_t S10;
	uint32_t S11;
	uint32_t S12;
	uint32_t S13;
	uint32_t S14;
	uint32_t S15;
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
#ifdef FEATURE_HFLOAT
	uint32_t S16;
	uint32_t S17;
	uint32_t S18;
	uint32_t S19;
	uint32_t S20;
	uint32_t S21;
	uint32_t S22;
	uint32_t S23;
	uint32_t S24;
	uint32_t S25;
	uint32_t S26;
	uint32_t S27;
	uint32_t S28;
	uint32_t S29;
	uint32_t S30;
	uint32_t S31;
#endif
	uint32_t kRetv;
}__attribute__((aligned(8)));




#endif /* TCHtch_port_H_ */
