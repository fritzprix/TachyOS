/*
 * tch_interrupt.c
 *
 *  Created on: 2015. 8. 18.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_interrupt.h"




DECLARE_SYSCALL_2(interrupt_enable,IRQn_Type,uint32_t,tchStatus);
DECLARE_SYSCALL_1(interrupt_disable,IRQn_Type,tchStatus);

void tch_remapTable(void* tblp){
	// TODO : remap isr vector table.
	/**
	 * Initial ISR vector table after reset can be generally used by bootloader or any early stage programs.
	 * if another ISR vector table should be used, there are solutions listed below.
	 *  1. Reprogram ISR vector table
	 *  2. Map new ISR Vector table (special H/W Support Required)
	 */
}

DEFINE_SYSCALL_2(interrupt_enable,IRQn_Type,irq,uint32_t,priority,tchStatus){
	NVIC_DisableIRQ(irq);
	NVIC_SetPriority(irq,priority);
	NVIC_EnableIRQ(irq);
	return tchOK;
}

DEFINE_SYSCALL_1(interrupt_disable,IRQn_Type,irq,tchStatus){
	NVIC_DisableIRQ(irq);
	return tchOK;
}

tchStatus tch_enableInterrupt(IRQn_Type irq,uint32_t priority){
	if(tch_port_isISR())
		return __interrupt_enable(irq,priority);
	return __SYSCALL_2(interrupt_enable,irq,priority);
}

tchStatus tch_disableInterrupt(IRQn_Type irq){
	if(tch_port_isISR())
		return __interrupt_disable(irq);
	return __SYSCALL_1(interrupt_disable,irq);
}

