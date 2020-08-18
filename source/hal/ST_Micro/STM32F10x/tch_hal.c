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

#include "tch_gpio.h"
#include "tch_timer.h"
#include "tch_port.h"
#include "tch_hal.h"
#include "tch_kernel.h"
#include "tch_mm.h"

#include "kernel/tch_interrupt.h"
#include "kernel/tch_err.h"


/**
 * 	void*                _hw;
	volatile uint32_t*   _clkenr;
	const uint32_t        clkmsk;
	volatile uint32_t*   _lpclkenr;
	const uint32_t        lpclkmsk;
	uint32_t              io_ocpstate;
 */
__TCH_STATIC_INIT tch_gpio_descriptor GPIO_HWs[] = {
		{
				._hw = GPIOA,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_IOPAEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOB,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_IOPBEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOC,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_IOPCEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOD,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_IOPDEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOE,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_IOPEEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				.io_ocpstate = 0
		}
};


__TCH_STATIC_INIT tch_ioInterrupt_descriptor IoInterrupt_HWs[] = {
		{
				.io_occp = 0,
				.irq = EXTI0_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI1_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI2_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI3_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI4_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI9_5_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI9_5_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI9_5_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI9_5_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI9_5_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		},
		{
				.io_occp = 0,
				.irq = EXTI15_10_IRQn
		}
};


/**
 */
__TCH_STATIC_INIT tch_dma_descriptor DMA_HWs[] ={
		{
				DMA1_Channel1,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				0,
				DMA1_Channel1_IRQn
		},
		{
				DMA1_Channel2,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				4,
				DMA1_Channel2_IRQn
		},
		{
				DMA1_Channel3,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				8,
				DMA1_Channel3_IRQn
		},
		{
				DMA1_Channel4,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				12,
				DMA1_Channel4_IRQn
		},
		{
				DMA1_Channel5,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				16,
				DMA1_Channel5_IRQn
		},
		{
				DMA1_Channel6,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				20,
				DMA1_Channel6_IRQn
		},
		{
				DMA1_Channel7,
				NULL,
				&RCC->AHBENR,
				RCC_AHBENR_DMA1EN,
				NULL,
				0,
				&DMA1->ISR,
				&DMA1->IFCR,
				24,
				DMA1_Channel7_IRQn
		}
};


/**
 */
__TCH_STATIC_INIT tch_uart_descriptor UART_HWs[MFEATURE_UART] = {
		{
				USART1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_USART1EN,
				NULL,
				0,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_USART1RST,
				USART1_IRQn
		},
		{
				USART2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_USART2EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_USART2RST,
				USART2_IRQn
		},
		{
				USART3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_USART3EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_USART3RST,
				USART3_IRQn
		},
		{
				UART4,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_UART4EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_UART4RST,
				UART4_IRQn
		}
};


/**
 * */
__TCH_STATIC_INIT tch_timer_descriptor TIMER_HWs[MFEATURE_TIMER] = {
		{
				TIM2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM2EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM2RST,
				4,
				16,
				0,
				TIM2_IRQn
		},
		{
				TIM3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM3EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM3RST,
				4,
				16,
				0,
				TIM3_IRQn
		},
		{
				TIM4,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM4EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM4RST,
				4,
				16,
				0,
				TIM4_IRQn
		},
		{
				TIM5,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM5EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM5RST,
				4,
				32,
				0,
				TIM5_IRQn
		},
		{
				TIM6,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM6EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM6RST,
				1,
				16,
				0,
				TIM6_IRQn
		},
		{
				TIM7,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM7EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM7RST,
				1,
				16,
				0,
				TIM7_IRQn
		}
};

__TCH_STATIC_INIT tch_spi_descriptor SPI_HWs[MFEATURE_SPI] = {
		{
				SPI1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_SPI1EN,
				NULL,
				0,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_SPI1RST,
				SPI1_IRQn
		},
		{
				SPI2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_SPI2EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_SPI2RST,
				SPI2_IRQn
		},
		{
				SPI3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_SPI3EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_SPI3RST,
				SPI3_IRQn
		}
};

__TCH_STATIC_INIT tch_iic_descriptor IIC_HWs[MFEATURE_IIC] = {
		{
				I2C1,
				NULL,
				0,
				&RCC->APB1ENR,
				RCC_APB1ENR_I2C1EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_I2C1RST,
				I2C1_EV_IRQn
		},
		{
				I2C2,
				NULL,
				0,
				&RCC->APB1ENR,
				RCC_APB1ENR_I2C2EN,
				NULL,
				0,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_I2C2RST,
				I2C2_EV_IRQn
		}
};



__TCH_STATIC_INIT tch_adc_descriptor ADC_HWs[MFEATURE_ADC] = {
		{
				ADC1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC1EN,
				NULL,
				0,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADC1RST,
				ADC1_2_IRQn
		},
		{
				ADC2,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC2EN,
				NULL,
				0,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADC2RST,
				ADC1_2_IRQn
		},
		{
				ADC3,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC3EN,
				NULL,
				0,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADC3RST,
				ADC3_IRQn
		}
};


//Placeholder for prevent build from failure
__TCH_STATIC_INIT tch_sdio_descriptor SDIO_HWs[MFEATURE_SDIO] = {
		{
				._sdio = SDIO,
				._handle = NULL,
				._clkenr = &RCC->AHBENR,
				.clkmsk = RCC_AHBENR_SDIOEN,
				._lpclkenr = NULL,
				.lpclkmsk = 0,
				._rstr = NULL,
				.rstmsk = 0,
				.irq = SDIO_IRQn
		}
};


void tch_hal_enableSystick(uint32_t mills){
	SysTick_Config(mills * (SYS_CLK / 1000));
	tch_port_enableInterrupt(SysTick_IRQn, PRIORITY_0, NULL);
}

void tch_hal_disableSystick(){
	tch_port_disableInterrupt(SysTick_IRQn);
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

void tch_hal_setSleepMode(tch_lplvl lplvl){
	SCB->SCR &= ~(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);
	PWR->CR &= ~PWR_CR_LPDS;
	switch(lplvl){
	case LP_LEVEL0:
		break;
	case LP_LEVEL1:
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		PWR->CR |= PWR_CR_LPDS;
		break;
	case LP_LEVEL2:
		SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
		PWR->CR |= (PWR_CR_LPDS);
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

void tch_hal_pauseSysClock(){
	/**
	 * STM32F4/2x handles main clock suspension when system enter sleep mode
	 */
}

void tch_hal_resumeSysClock(){
	uint32_t tmp = 0;
	RCC->CR |= RCC_CR_HSEON;
	while(!(RCC->CR & RCC_CR_HSERDY))
		__NOP();

	RCC->CFGR |= RCC_CFGR_PLLSRC_HSE;
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
	tch_kernel_onSystick();
}
