/*
 * tch_hal.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#include "hal/tch_hal.h"
#include "tch_kernel.h"




tch_hal tch_hal_instance;

const tch_hal* tch_kernel_initHAL(){
	/***
	 *  initialize clock subsystem
	 */
	SystemInit();

	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;

	/***
	 *  bind hal interface
	 */
	tch_hal_instance.adc = tch_adc_instance;

#if MFEATURE_GPIO
	tch_hal_instance.gpio  = tch_gpio_instance;
#endif

	tch_hal_instance.timer = tch_timer_instance;
	tch_hal_instance.usart = tch_usart_instance;
	tch_hal_instance.spi = tch_spi_instance;
	tch_hal_instance.i2c = tch_i2c_instance;
	return &tch_hal_instance;
}


void tch_hal_enableSystick(){
	SysTick_Config(SYS_CLK / 1000);
	NVIC_DisableIRQ(SysTick_IRQn);
	NVIC_SetPriority(SysTick_IRQn,HANDLER_SYSTICK_PRIOR);
	NVIC_EnableIRQ(SysTick_IRQn);
}

void tch_hal_disableSystick(){
	NVIC_DisableIRQ(SysTick_IRQn);
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}


void tch_hal_setSleepMode(tch_lplvl lplvl){
	SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
	PWR->CR &= ~(PWR_CR_LPDS | PWR_CR_FPDS);
	switch(lplvl){
	case LP_LEVEL0:
		break;
	case LP_LEVEL1:
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		PWR->CR |= PWR_CR_LPDS;
		break;
	case LP_LEVEL2:
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		PWR->CR |= (PWR_CR_LPDS | PWR_CR_FPDS);
		break;
	}
}

void tch_hal_enterSleepMode(){
	__DMB();
	__ISB();
	__WFI();
	__DMB();
	__ISB();
}

void tch_hal_suspendSysClock(){

}

void tch_hal_resumeSysClock(){
	uint32_t tmp = 0;
	RCC->CR |= RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY))
		__NOP();

	RCC->PLLCFGR |= RCC_PLLCFGR_PLLSRC_HSE;
	RCC->CR |= RCC_CR_PLLON;
	while(!(RCC->CR & RCC_CR_PLLRDY))
		__NOP();
	tmp = RCC->CFGR;
	tmp &= ~RCC_CFGR_SW;
	tmp |= RCC_CFGR_SW_PLL;
	RCC->CFGR = tmp;
	while((RCC->CFGR & RCC_CFGR_SWS_PLL) != RCC_CFGR_SWS_PLL)
		__NOP();
}


void SysTick_Handler(void){
	tch_KernelOnSystick();
}
