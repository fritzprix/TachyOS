/*
 * tch_ptypes.h
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */

#ifndef TCH_PTYPES_H_
#define TCH_PTYPES_H_

#if defined(__cplusplus)
extern "C"{
#endif


#include <stdint.h>

/*
 *
 *  must be defined properly your target platform
 *
 */
typedef uint32_t uword_t;
typedef uint16_t uhword_t;
typedef int32_t  word_t;
typedef int16_t  hword_t;
typedef void* uaddr_t;

#define WORD_MAX ((uint32_t) -1)



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


#if defined(__cplusplus)
}
#endif

#endif /* TCH_PTYPES_H_ */
