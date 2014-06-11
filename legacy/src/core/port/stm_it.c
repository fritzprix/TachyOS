/*
 * stm_it.c
 *
 *  Created on: 2014. 3. 27.
 *      Author: innocentevil
 */

#include "stm32f4xx.h"

//extern void inline  __isr_epilogue(void)__attribute__((always_inline));
extern void Systick_Vector(void);
//extern void inline __isr_prologue(void)__attribute__((always_inline));

void NMI_Handler(void){
	while(1);
}
void HardFault_Handler(void){
	while(1);
}

void MemManage_Handler(void){
	while(1);
}

void BusFault_Handler(void){
	while(1);
}

void UsageFault_Handler(void){
	while(1);
}


void SysTick_Handler(void){
	Systick_Vector();
}


