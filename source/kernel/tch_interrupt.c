/*
 * tch_interrupt.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 8. 18.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_interrupt.h"




DECLARE_SYSCALL_3(interrupt_enable,IRQn_Type,uint32_t,isr_handler,tchStatus);
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

DEFINE_SYSCALL_3(interrupt_enable,IRQn_Type,irq,uint32_t,priority,isr_handler, handler,tchStatus){
	tch_port_enableInterrupt(irq,priority,handler);
	return tchOK;
}

DEFINE_SYSCALL_1(interrupt_disable,IRQn_Type,irq,tchStatus){
	tch_port_disableInterrupt(irq);
	return tchOK;
}

tchStatus tch_enableInterrupt(IRQn_Type irq,uint32_t priority,isr_handler handler){
	if(tch_port_isISR())
		return __interrupt_enable(irq,priority,handler);
	return __SYSCALL_2(interrupt_enable,irq,priority);
}

tchStatus tch_disableInterrupt(IRQn_Type irq){
	if(tch_port_isISR())
		return __interrupt_disable(irq);
	return __SYSCALL_1(interrupt_disable,irq);
}

