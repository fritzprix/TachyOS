/*
 * tch_port.c
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

#include <stdio.h>
#include <stdlib.h>

#include "tch_kernel.h"
#include "tch_hal.h"



#define IDLE_STACK_SIZE            (uint32_t) (1 << 9)
#define MAIN_STACK_SIZE            (uint32_t) (1 << 11)

#define SCB_AIRCR_KEY              (uint32_t) (0x5FA << SCB_AIRCR_VECTKEY_Pos)
#define EPSR_THUMB_MODE            (uint32_t) (1 << 24)

#define CTRL_PSTACK_ENABLE         (uint32_t) (1 << 1)
#define CTRL_FPCA                  (uint32_t) (1 << 2)

static void __pend_loop(void) __attribute__((naked));
//static int isr_svc_cnt;


BOOL tch_kernel_initPort(){
	__disable_irq();
	//isr_svc_cnt = 0;
	SCB->AIRCR = (SCB_AIRCR_KEY | (6 << SCB_AIRCR_PRIGROUP_Pos));          /**  Set priority group
	                                                                        *   - [7] : Group Priority / [6:4] : Subpriority
	                                                                        *   - Handler or thread within same group priority
	                                                                        *     don't preempt each other
	                                                                        *   - Kenel thread has group priorty 1
	                                                                        *   - SuperVisor Mode has group priority 0
	                                                                        *   - highest priorty isr has group priority 1
	                                                                        *   -> Kernel thread isn't preempted by other isr
	                                                                        **/
//	SCB->CCR |= SCB_CCR_NONBASETHRDENA_Msk;
	SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk);    /**
	                                                                                                       *  General Fault handler enable
	                                                                                                       *  - for debugging convinience
	                                                                                                       **/

	__set_PSP(__get_MSP());                         // Init Stack inherited to thread mode stack pointer
	uint32_t mcu_ctrl = __get_CONTROL();            /** Modify Control register
	                                                 *  - dedicated Thread Stack Pointer enabled
	                                                 *
	                                                 **/


	mcu_ctrl |= CTRL_PSTACK_ENABLE;
#ifdef MFEATURE_HFLOAT
	/***
	 *   FPU Activation
	 */
	FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
	SCB->CPACR |= (0xF << 20);
#endif
	__set_CONTROL(mcu_ctrl);
	__ISB();
	__DMB();

	// set Handler Priority

	NVIC_SetPriority(SysTick_IRQn,HANDLER_SYSTICK_PRIOR);
	NVIC_SetPriority(SVCall_IRQn,HANDLER_SVC_PRIOR);
	NVIC_SetPriority(PendSV_IRQn,HANDLER_SVC_PRIOR);
	NVIC_EnableIRQ(SysTick_IRQn);
	NVIC_EnableIRQ(SVCall_IRQn);
	NVIC_EnableIRQ(PendSV_IRQn);


	return TRUE;
}


void tch_port_enableSysTick(void){
	SysTick_Config(SYS_CLK / 1000);
}


void tch_port_kernel_lock(void){
	__set_BASEPRI(MODE_KERNEL);
}


void tch_port_kernel_unlock(void){
	__set_BASEPRI(MODE_USER);
}

BOOL tch_port_isISR(){
	return __get_IPSR() > 0;
}


void tch_port_enableISR(void){
	__enable_irq();
}

void tch_port_disableISR(void){
	__disable_irq();
}



void tch_port_switchContext(uaddr_t nth,uaddr_t cth){
	//isr_svc_cnt--;
	asm volatile(
#ifdef MFEATURE_HFLOAT
			"vpush {s16-s31}\n"
#endif
			"push {r4-r11,lr}\n"                 ///< save thread context in the thread stack
			"str sp,[%0]\n"                      ///< store

			"ldr sp,[%1]\n"
			"pop {r4-r11,lr}\n"
#ifdef MFEATURE_HFLOAT
			"vpop {s16-s31}\n"
#endif
			"ldr r0,=%2\n"
			"svc #0" : : "r"(&((tch_thread_header*) cth)->t_ctx),"r"(&((tch_thread_header*) nth)->t_ctx),"i"(SV_EXIT_FROM_SV):);
}


/***
 *  this function redirect execution to thread mode for thread context manipulation
 */
void tch_port_jmpToKernelModeThread(uaddr_t routine,uword_t arg1,uword_t arg2,uword_t ret_val){
	//if(isr_svc_cnt)
//		tch_kernel_errorHandler(FALSE,osErrorISR);
	//isr_svc_cnt++;
	tch_exc_stack* org_sp = (tch_exc_stack*)__get_PSP();          //
	                                                              //   prepare fake exception stack
	                                                              //    - passing arguement to kernel mode thread
	                                                              //   - redirect kernel routine
	                                                              //
	org_sp--;                                                     // 1. push stack
	memset(org_sp,0,sizeof(tch_exc_stack));
	org_sp->R0 = arg1;                                            // 2. pass arguement into fake stack
	org_sp->R1 = arg2;
	tch_kernelSetResult(arg1,ret_val);
	                                                              //
	                                                              //  kernel thread function has responsibility to push r12 in stack of thread
	                                                              //  so when this pended thread restores its context, kernel thread result could be retrived from saved stack
	                                                              //  more detail could be found in context switch function

	org_sp->Return = (uint32_t)routine;                           // 3. modify return address of exc entry stack
	org_sp->xPSR = EPSR_THUMB_MODE;                               // 4. ensure returning to thumb mode
	__set_PSP((uint32_t)org_sp);                                  // 5. set manpulated exception stack as thread stack pointer
	tch_port_kernel_lock();                                       // 6. finally lock as kernel execution
}



int tch_port_enterSvFromUsr(word_t sv_id,uword_t arg1,uword_t arg2){
	asm volatile(
			"dmb\n"
			"isb\n"
			"svc #0"   : : :);        // return from sv interrupt and get result from register #0
	return ((tch_thread_header*)tch_currentThread)->t_kRet;
}
/***
 */
int tch_port_enterSvFromIsr(word_t sv_id,uword_t arg1,uword_t arg2){
	if(SCB->ICSR & SCB_ICSR_PENDSVSET_Msk)
		tch_kernel_errorHandler(FALSE,osErrorISRRecursive);
	tch_exc_stack* org_sp = (tch_exc_stack*) __get_PSP();
	org_sp--;                                              // push stack to prepare manipulated stack for passing arguements to sv call(or handler)
	org_sp->R0 = sv_id;
	org_sp->R1 = arg1;
	org_sp->R2 = arg2;
	org_sp->Return = (uint32_t)__pend_loop;
	org_sp->xPSR = EPSR_THUMB_MODE;
	__set_PSP((uint32_t)org_sp);
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	return 0;
}

/**
 *  prepare initial context for start thread
 */
void* tch_port_makeInitialContext(uaddr_t th_header,uaddr_t initfn){
	tch_exc_stack* exc_sp = (tch_exc_stack*) th_header - 1;                // offset exc_stack size (size depends on floating point option)
	memset(exc_sp,0,sizeof(tch_exc_stack));
	exc_sp->Return = (uint32_t)initfn;
	exc_sp->xPSR = EPSR_THUMB_MODE;
	exc_sp->R0 = th_header;
	exc_sp->R1 = osOK;
#if MFEATURE_HFLOAT
	exc_sp->S0 = (float)0.2f;
#endif
	exc_sp = ((uint32_t*) exc_sp + 2);

	tch_thread_context* th_ctx = (tch_thread_context*) exc_sp - 1;
	memset(th_ctx,0,sizeof(tch_thread_context) - 8);
	return (uint32_t*) th_ctx;

}

int tch_port_atomicCompareModify(uaddr_t dval,uaddr_t tval){
	volatile int result = 0;
	asm volatile(
			"LDR r4,[r0]\n"
			"LDR r3,=#1\n"
			"TRY:\n"
			"LDREX r5,[r1]\n"
			"CMP r5,r4\n"
			"ITTEE NE\n"
			"STREXNE r6,r5,[r0]\n"
			"ADDNE r3,r3,r3\n"
			"STREXEQ r6,r4,[r1]\n"
			"SUBEQ r3,r3,r3\n"
			"CMP r6,#0\n"
			"BNE TRY\n"
			"STR r3,[%0]" : : "r"(&result) : "r4","r5","r6","r3");
	return result;
}

int tch_port_exclusiveCompareUpdate(uaddr_t dest,uword_t comp,uword_t update){
	int result = 0;
	asm volatile(
			"push {r4-r6}\n"
			"__exCmpUpdate:\n"
			"LDREX r4,[r0]\n"       // read dest exclusively
			"CMP r4,r1\n"           // compare read val to comp
			"ITTEE EQ\n"            // if not equal
			"STREXEQ r6,r2,[r0]\n"  // update dest with new one
			"LDREQ r5,=#0\n"        // return 1
			"STREXNE r6,r4,[r0]\n"  // else update previous value
			"LDRNE r5,=#1\n"        // return 0
			"CMP r6,#0\n"
			"BNE __exCmpUpdate\n"
			"STR r5,[%0]\n"
			"pop {r4-r6}" : : "r"(&result) : "r4","r5","r6");
	return result;
}

int tch_port_exclusiveCompareDecrement(uaddr_t dest,uword_t comp){
	int result = 0;
	asm volatile(
			"push {r4-r6}\n"
			"_exCmpDec:\n"
			"LDREX r4,[r0]\n"
			"CMP r4,r1\n"
			"ITTE NE\n"
			"SUBNE r4,r4,#1\n"
			"LDRNE r5,=#1\n"
			"LDREQ r5,=#0\n"
			"STREX r6,r4,[r0]\n"
			"CMP r6,#0\n"
			"BNE _exCmpDec\n"
			"STR r5,[%0]\n"
			"pop {r4-r6}\n" : :"r"(&result) : "r4","r5","r6");
	return result;
}

void __pend_loop(void){
	__ISB();
	__DMB();
	__WFI();
}

void SysTick_Handler(void){
	tch_kernelSysTick();
}


void SVC_Handler(void){
	tch_exc_stack* exsp = (tch_exc_stack*)__get_PSP();
	tch_kernelSvCall(exsp->R0,exsp->R1,exsp->R2);
}


void PendSV_Handler(void){
	tch_exc_stack* exsp = (tch_exc_stack*)__get_PSP();
	tch_exc_stack* org_sp = exsp + 1;
	__set_PSP((uint32_t)org_sp);                                       // discard stack used to transfer arguements
	tch_kernelSvCall(exsp->R0,exsp->R1,exsp->R2);
}


void HardFault_Handler(){
	tch_kernel_faulthandle(FAULT_TYPE_HARD);
}

void MemManage_Handler(){
	tch_kernel_faulthandle(FAULT_TYPE_MEM);
}

void BusFault_Handler(){
	tch_kernel_faulthandle(FAULT_TYPE_BUS);
}

void UsageFault_Handler(){
	tch_kernel_faulthandle(FAULT_TYPE_USG);
}
