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
#include "tch_fault.h"
#include "tch_hal.h"
#include "tch_port.h"
#include "tch_ptypes.h"



#define SCB_AIRCR_KEY              ((uint32_t) (0x5FA << SCB_AIRCR_VECTKEY_Pos))
#define EPSR_THUMB_MODE            ((uint32_t) (1 << 24))

#define CTRL_PSTACK_ENABLE         ((uint32_t) 0x2)
#define CTRL_UNPRIV_THREAD_ENABLE  ((uint32_t) 0x1)
#define CTRL_FPCA                  ((uint32_t) 0x4)

#define PE_TEXT						((uint8_t) 0)
#define PE_DATA						((uint8_t) 1)
#define PE_STACK					((uint8_t) 2)
#define PE_DYNAMIC					((uint8_t) 3)


#define NR_PAGE_ENTRY				8



struct pte  {
	union {
		struct {
			uint32_t	baddr;
			uint32_t	attr;
		}__attribute__((packed));
		uint64_t value;
	};
};

struct pgd {
	uint8_t		dynamic_idx;
	struct pte 	_pte[8];
};


BOOL tch_port_init(){
	__disable_irq();
	SCB->AIRCR = (SCB_AIRCR_KEY | (6 << SCB_AIRCR_PRIGROUP_Pos));          /**  Set priority group
	                                                                        *   - [7] : Group Priority / [6:4] : Subpriority
	                                                                        *   - Handler or thread within same group priority
	                                                                        *     don't preempt each other
	                                                                        *   - Kenel thread has group priorty 1
	                                                                        *   - SuperVisor Mode has group priority 0
	                                                                        *   - highest priorty isr has group priority 1
	                                                                        *   -> Kernel thread isn't preempted by other isr
	                                                                        **/
	SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk);    /**
	                                                                                                       *  General Fault handler enable
	                                                                                                       *  - for debugging convinience
	                                                                                                       **/
	__set_PSP(__get_MSP());                         // Init Stack inherited to thread mode stack pointer
	uint32_t mcu_ctrl = __get_CONTROL();            /** Modify Control register
	                                                 *  - dedicated Thread Stack Pointer enabled
	                                                 *
	                                                 **/

#ifdef __DBG
	DBGMCU->CR |= (DBGMCU_CR_DBG_SLEEP | DBGMCU_CR_DBG_STOP);
#endif
	mcu_ctrl |= CTRL_PSTACK_ENABLE;
#if FEATURE_FLOAT > 0
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

	NVIC_SetPriority(SVCall_IRQn,HANDLER_SVC_PRIOR);
	NVIC_SetPriority(PendSV_IRQn,HANDLER_SVC_PRIOR);
	NVIC_EnableIRQ(SVCall_IRQn);
	NVIC_EnableIRQ(PendSV_IRQn);

	return TRUE;
}


void tch_port_setIsrVectorMap(uint32_t isrv){
	isrv &= ~0x200;
	SCB->VTOR = isrv;
}


void tch_port_enablePrivilegedThread(){
	uint32_t mcu_control = __get_CONTROL();
	mcu_control &= ~CTRL_UNPRIV_THREAD_ENABLE;
	__set_CONTROL(mcu_control);
}

void tch_port_disablePrivilegedThread(){
	uint32_t mcu_control = __get_CONTROL();
	mcu_control |= CTRL_UNPRIV_THREAD_ENABLE;
	__set_CONTROL(mcu_control);
}


void tch_port_atomicBegin(void){
	__set_BASEPRI(MODE_KERNEL);
}


void tch_port_atomicEnd(void){
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

void tch_port_switch(uaddr_t nth,uaddr_t cth){
	asm volatile(
#if FEATURE_FLOAT > 0
			"vpush {s16-s31}\n"
#endif
			"push {r4-r11,lr}\n"                 ///< save thread context in the thread stack
			"str sp,[%0]\n"                      ///< store

			"ldr sp,[%1]\n"
			"pop {r4-r11,lr}\n"
#if FEATURE_FLOAT > 0
			"vpop {s16-s31}\n"
#endif
			"ldr r0,=%2\n"
			"svc #0" : : "r"(&((tch_thread_kheader*) cth)->ctx),"r"(&((tch_thread_kheader*) nth)->ctx),"i"(SV_EXIT_FROM_SV) :"r4","r5","r6","r8","r9","r10","lr");
}


/***
 *  this function redirect execution to thread mode for thread context manipulation
 */
void tch_port_enterPrivThread(uaddr_t routine,uword_t arg1,uword_t arg2,uword_t arg3){
	tch_exc_stack* org_sp = (tch_exc_stack*)__get_PSP();          //
	                                                              //   prepare fake exception stack
	                                                              //    - passing arguement to kernel mode thread
	                                                              //   - redirect kernel routine
	                                                              //
	org_sp--;                                                     // 1. push stack
	mset(org_sp,0,sizeof(tch_exc_stack));
	org_sp->R0 = arg1;                                            // 2. pass arguement into fake stack
	org_sp->R1 = arg2;
	org_sp->R2 = arg3;

	__VALID_SYSCALL = TRUE;
	                                                              //
	                                                              //  kernel thread function has responsibility to push r12 in stack of thread
	                                                              //  so when this pended thread restores its context, kernel thread result could be retrived from saved stack
	                                                              //  more detail could be found in context switch function

	org_sp->Return = (uint32_t)routine;                           // 3. modify return address of exc entry stack
	org_sp->xPSR = EPSR_THUMB_MODE;                               // 4. ensure returning to thumb mode
	__set_PSP((uint32_t)org_sp);                                  // 5. set manpulated exception stack as thread stack pointer
	__DMB();
	__ISB();
	tch_port_enablePrivilegedThread();
	tch_port_atomicBegin();                                       // 6. finally lock as kernel execution
}



int tch_port_enterSv(word_t sv_id,uword_t arg1,uword_t arg2,uword_t arg3){
	asm volatile(
			"ldr r4,=#1\n"
			"str r4,[%0]\n"
			"dmb\n"
			"isb\n"
			"svc #0"  :  : "r"(&__VALID_SYSCALL) : "r0","r1","r2","r3","r4" );        // return from sv interrupt and get result from register #0
	return ((tch_thread_uheader*)current)->kRet;
}


/**
 *  prepare initial context for start thread
 */
void* tch_port_makeInitialContext(uaddr_t uthread_header,uaddr_t stktop,uaddr_t initfn){
	tch_exc_stack* exc_sp = (tch_exc_stack*) stktop - 1;                // offset exc_stack size (size depends on floating point option)
	exc_sp = (tch_exc_stack*)((int) exc_sp & ~7);
	mset(exc_sp,0,sizeof(tch_exc_stack));
	exc_sp->Return = (uint32_t)initfn;
	exc_sp->xPSR = EPSR_THUMB_MODE;
	exc_sp->R0 = (uint32_t) uthread_header;
	exc_sp->R1 = tchOK;
#if FEATURE_FLOAT > 0
	exc_sp->S0 = (float)0.2f;
#endif
	exc_sp = (tch_exc_stack*)((uint32_t*) exc_sp + 2);

	tch_thread_context* th_ctx = (tch_thread_context*) exc_sp - 1;
	mset(th_ctx,0,sizeof(tch_thread_context) - 8);
	return (uint32_t*) th_ctx;

}
/**
 * read
 */
int tch_port_exclusiveCompareUpdate(uaddr_t dest,uword_t comp,uword_t update){
	int result = 0;
	asm volatile(
			"push {r4-r6}\n"
			"__exCmpUpdate:\n"
			"LDREX r4,[r0]\n"       // read dest exclusively
			"CMP r4,r1\n"           // compare read val to comp
			"ITTEE EQ\n"            // if equal
			"STREXEQ r6,r2,[r0]\n"  // update dest with new one
			"LDREQ r5,=#1\n"        // return 0
			"STREXNE r6,r4,[r0]\n"  // else update previous value
			"LDRNE r5,=#0\n"        // return 1
			"CMP r6,#0\n"
			"BNE __exCmpUpdate\n"
			"mov %0,r5\n"
			"pop {r4-r6}" : "=r"(result) :  : "r4","r5","r6");
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
			"mov %0,r5\n"
			"pop {r4-r6}\n" : "=r"(result) : : "r4","r5","r6");
	return result;
}



void SVC_Handler(void){
	tch_exc_stack* exsp = (tch_exc_stack*)__get_PSP();
	tch_kernel_onSyscall(exsp->R0,exsp->R1,exsp->R2,exsp->R3);
}

int tch_port_clearFault(int type){
	switch(type){
	case FAULT_HARD:
		break;
	case FAULT_BUS:
		break;
	case FAULT_ILLSTATE:
		break;
	}
	return 0;
}

void inline tch_port_updateProtectionEntry(struct pte* ptep){
	MPU->RNR = ptep->baddr & MPU_RBAR_REGION_Msk;
	MPU->RASR &= ~MPU_RASR_ENABLE_Msk;			// disable mpu region

	MPU->RBAR = ptep->baddr;					// setup base address
	MPU->RASR = ptep->attr;						// setup attribute
	__ISB();
}

int tch_port_reset(){
	return 0;
}

void tch_port_loadPageTable(pgd_t* pgd){
	struct pgd* pgdp = (struct pgd*) pgd;
	uint8_t idx = 0;
	for(;idx < NR_PAGE_ENTRY;idx++){
		tch_port_updateProtectionEntry(&pgdp->_pte[idx]);
	}
}


pgd_t* tch_port_allocPageDirectory(kernel_alloc_t alloc){
	struct pgd* pgdp = alloc(sizeof(struct pgd));
	uint8_t i;
	for(i = 0;i < NR_PAGE_ENTRY;i++){
		pgdp->_pte[i].value = -1;
	}
	pgdp->dynamic_idx = 0;
	return (pgd_t*) pgdp;
}

int tch_port_addPageEntry(pgd_t* pgd,uint32_t poffset,uint32_t flag){
	if(!pgd || !poffset)
		return FALSE;
	struct pgd* pgdp = (struct pgd*) pgd;

	uint8_t idx;
	switch(flag & SECTION_MSK){
	case SECTION_DYNAMIC:
		// check page is already in the table
		for(idx = PE_DYNAMIC;idx < NR_PAGE_ENTRY;idx++){
			if(((uint32_t)pgdp->_pte[idx].baddr & MPU_RBAR_ADDR_Msk) == ((uint32_t)poffset << PAGE_OFFSET)){
				return FALSE;
			}
		}
		idx = pgdp->dynamic_idx + PE_DYNAMIC;
		pgdp->dynamic_idx++;
		if(pgdp->dynamic_idx >= NR_PAGE_ENTRY)
			pgdp->dynamic_idx = PE_DYNAMIC;
		break;
	case SECTION_TEXT:
		idx = PE_TEXT;
		break;
	case SECTION_STACK:
		idx = PE_STACK;
		break;
	case SECTION_DATA:
		idx = PE_DATA;
		break;
	}

	uint32_t address = 0;
	uint32_t attr = 0;
	pgdp->_pte[idx].value = 0;

	attr &= ~MPU_RASR_TEX_Msk;
	if(flag & SEGMENT_DEVICE) {
		attr |= (MPU_RASR_B_Msk | MPU_RASR_S_Msk);
	} else {
		switch (get_memtype(flag)) {
		case MEMTYPE_INROM:
			attr |= MPU_RASR_C_Msk;
			break;
		case MEMTYPE_EXROM:
			attr |= MPU_RASR_C_Msk;
			break;
		case MEMTYPE_INRAM:
			attr |= (MPU_RASR_C_Msk | MPU_RASR_S_Msk);
			break;
		case MEMTYPE_EXRAM:
			attr |= (MPU_RASR_C_Msk | MPU_RASR_B_Msk | MPU_RASR_S_Msk);
			break;
		}
	}

	attr |= (flag & (PERM_KERNEL_XC | PERM_OTHER_XC | PERM_OWNER_XC)) ? 0 : MPU_RASR_XN_Msk;	// instruction fetch
	if(!(flag & (PERM_KERNEL_WR | PERM_OWNER_WR)))
		attr |= (4 << MPU_RASR_AP_Pos);				// read only
	attr |= (2 << MPU_RASR_AP_Pos);
	attr |= (flag & PERM_OWNER_WR) ? (1 << MPU_RASR_AP_Pos) : 0;
	attr |= (MPU_RASR_SIZE_Msk & PAGE_OFFSET);
	attr |= MPU_RASR_ENABLE_Msk;

	address = ((uint32_t) poffset << PAGE_OFFSET)  | (idx & MPU_RBAR_REGION_Msk)/* | MPU_RBAR_VALID_Msk*/;		// region valid is cleared

	pgdp->_pte[idx].baddr = address;
	pgdp->_pte[idx].attr = attr;

	__ISB();
	__DMB();

	tch_port_updateProtectionEntry(&pgdp->_pte[idx]);
	idx++;
	return TRUE;
}

int tch_port_removePageEntry(pgd_t* pgd,uint32_t poffset){
	uint8_t idx;
	struct pgd* pgdp = (struct pgd*) pgd;
	struct pte* ptep;
	for(idx = 0;idx < NR_PAGE_ENTRY; idx++) {
		ptep = &pgdp->_pte[idx];
		if(((uint32_t)ptep->baddr & MPU_RBAR_ADDR_Msk) == ((uint32_t)poffset << PAGE_OFFSET)){
			ptep->attr &= ~MPU_RASR_ENABLE_Msk;			// clear enable bit
			tch_port_updateProtectionEntry(ptep);
			ptep->value = 0;
			return idx;
		}
	}
	return -1;
}


void HardFault_Handler(){
	tch_kernel_onHardException(FAULT_HARD,SPEC_UND);
}

void MemManage_Handler(){
	uint32_t mfault = (SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) >> SCB_CFSR_MEMFAULTSR_Pos;
	paddr_t pa = 0;
	int spec = 0;
	if(mfault & 2)
		spec = SPEC_DACCESS;
	else if(mfault & 1)
		spec = SPEC_IACCESS;
	else if(mfault & (1 << 4))
		spec = SPEC_STK;
	else if(mfault & (1 << 3))
		spec = SPEC_UNSTK;
	if(mfault & (1 << 7))	// if fault address is valid, handled in page fault
	{
		pa = (paddr_t) SCB->MMFAR;
		tch_kernel_onMemFault(pa,FAULT_MEM,spec);
	}
	else
	{
		tch_kernel_onHardException(FAULT_MEM, spec);
	}
}

void BusFault_Handler(){
	uint32_t bfault = (SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) >> SCB_CFSR_BUSFAULTSR_Pos;
	int spec = 0;
	if(bfault & (1 << 4))
		spec = SPEC_STK;
	else if(bfault & (1 << 3))
		spec = SPEC_UNSTK;
	else if(bfault & (1 << 2))
		spec = SPEC_UND;
	else if(bfault & 2)
		spec = SPEC_DACCESS;
	else if(bfault & 1)
		spec = SPEC_IACCESS;
	tch_kernel_onHardException(FAULT_BUS,spec);
}

void UsageFault_Handler(){
	tch_kernel_onHardException(FAULT_ILLSTATE);
}
