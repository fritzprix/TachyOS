/*
 * tch_port.h
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#ifndef TCH_PORT_H_
#define TCH_PORT_H_


#include "tch.h"
#include "tch_portcfg.h"
#include "core_cm4.h"


#define _port_setThreadSP(sp)    __set_PSP(sp)
#define _port_getThreadSP()      __get_PSP()
#define _port_setHandlerSP(sp)   __set_MSP(sp)
#define _port_getHandlerSP()     __get_MSP()

typedef struct _tch_exc_stack_t tch_exc_stack;
typedef struct _tch_thread_context_t tch_thread_context;
/***
 *  port interface
 */
typedef struct tch_port_ix {

	void (*_enableSysTick)(void);
	void (*_enableISR)(void);
	void (*_disableISR)(void);
	/***
	 *  Kernal lock action
	 *   - to guarantee kernel operation not interrupted or preempted
	 */
	void (*_kernel_lock)(void);
	/***
	 *  Kernel Unlock action
	 *   - to allow interrupt or thread preemption when it's needed
	 */
	void (*_kernel_unlock)(void);
	void (*_switchContext)(void* nth,void* cth);
	void (*_saveContext)(void* cth);
	void (*_restoreContext)(void* cth);
	void (*_jmpToKernelModeThread)(void* routine,uint32_t arg1,uint32_t arg2);
	int (*_enterSvFromUsr)(int sv_id,uint32_t arg1,uint32_t arg2);
	int (*_enterSvFromIsr)(int sv_id,uint32_t arg1,uint32_t arg2);
	void* (*_makeInitialContext)(void* sp,void* initfn);
	void (*_setThreadSP)(uint32_t sp);
	uint32_t (*_getThreadSP)(void);
	void (*_setHandlerSP)(uint32_t sp);
	uint32_t (*_getHandlerSP)(void);
}tch_port_ix;

const tch_port_ix* tch_port_init();


typedef struct _arm_sbrtn_ctx arm_sbrtn_ctx;




/**
 * follows arm v7m arch. ref.
 * exception push & pop registers below in the stack at the entry and exit
 */

struct _tch_exc_stack_t {
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
}__attribute__((aligned(4)));

struct _tch_thread_context_t {
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
}__attribute__((aligned(4)));




#endif /* TCH_PORT_H_ */
