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

#include "tch_types.h"

#define tch_port_setUserSP(sp)          asm volatile ("MSR psp, %0\n" : : "r" (sp) : "sp")
#define tch_port_getUserSP()            __get_PSP()
#define tch_port_setKernelSP(sp)        asm volatile ("MSR msp, %0\n" : : "r" (sp) : "sp")
#define tch_port_getKernelSP()          __get_MSP()
/*
 *
 *  must be defined properly your target platform
 *
 */
typedef uint32_t uword_t;
typedef uint16_t uhword_t;
typedef int32_t  word_t;
typedef int16_t  hword_t;
typedef void* 	uwaddr_t;
typedef void*	paddr_t;



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
#if FEATURE_FLOAT > 0
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
#if FEATURE_FLOAT > 0
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
};


#endif /* TCHtch_port_H_ */
