/*
 * tch_intr.c
 *
 *  Created on: 2014. 8. 15.
 *      Author: innocentevil
 */


#include "tch_intr.h"
#include "tch_ktypes.h"
#include "tch_port.h"


#define INIT_HANDLER_PRIOR    {HANDLER_HIGH_PRIOR,HANDLER_NORMAL_PRIOR,HANDLER_LOW_PRIOR}

static void tch_intr_enable(uint32_t irq);
static void tch_intr_disable(uint32_t irq);
static void tch_intr_setPriority(uint32_t irq,uint32_t priorty);
static uint32_t tch_intr_getPriority(uint32_t irq);


__attribute__((section(".data"))) static tch_lld_intr INTERRUPT_StaticInstance = {
		INIT_HANDLER_PRIOR,
		tch_intr_enable,
		tch_intr_disable,
		tch_intr_setPriority,
		tch_intr_getPriority
};

const tch_lld_intr* tch_interrupt_instance = &INTERRUPT_StaticInstance;


static void tch_intr_enable(uint32_t irq){
	NVIC_EnableIRQ((IRQn_Type)irq);
}

static void tch_intr_disable(uint32_t irq){
	NVIC_DisableIRQ((IRQn_Type)irq);
}

static void tch_intr_setPriority(uint32_t irq,uint32_t priorty){
	NVIC_SetPriority((IRQn_Type)irq,priorty);
}

static uint32_t tch_intr_getPriority(uint32_t irq){
	return NVIC_GetPriority((IRQn_Type)irq);
}
