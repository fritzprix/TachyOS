/*
 * cortex_v7m_port.c
 *
 *  Created on: 2014. 4. 12.
 *      Author: innocentevil
 */


#include "cortex_v7m_port.h"
#include "../fmo_sched.h"


static void __pndsv_wloop(void);
static int pndrq = 0;
static int lstpndrq = 0;

int __sv_call(int8_t svc_type, uint32_t arg0,uint32_t arg1){
	uint32_t result = 0;
	asm volatile(
			"dmb\n"
			"isb\n"
			"svc #0\n"
			"str r0,[%0]" : : "r"(&result):);
	return result;
}


void __pndsv_handled(void){
	lstpndrq = pndrq;
	pndrq = 0;
}

int __pndsv_call(int8_t svc_type,uint32_t arg0,uint32_t arg1){
	arm_exc_stack* stack = (arm_exc_stack*)__get_PSP();
	pndrq++;
	KERNEL_LOCK();
	stack--;
	stack->R0 = svc_type;
	stack->R1 = (uint32_t) arg0;
	stack->R2 = (uint32_t) arg1;
	stack->Return =  (uint32_t)__pndsv_wloop;
	stack->xPSR = EPSR_THUMB_MODE;
	__set_PSP((uint32_t) stack);
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	__DMB();
	__ISB();
	return 0;
}

void __pndsv_wloop(void){
	__DMB();
	__ISB();
	__WFI();
}
