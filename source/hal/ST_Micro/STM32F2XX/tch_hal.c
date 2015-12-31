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
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIOAEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIOALPEN,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOB,
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIOBEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIOBLPEN,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOC,
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIOCEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIOCLPEN,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOD,
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIODEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIODLPEN,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOE,
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIOEEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIOELPEN,
				.io_ocpstate = 0
		},
		{
				._hw = GPIOF,
				._clkenr = &RCC->AHB1ENR,
				.clkmsk = RCC_AHB1ENR_GPIOFEN,
				._lpclkenr = &RCC->AHB1LPENR,
				.lpclkmsk = RCC_AHB1LPENR_GPIOFLPEN,
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
				DMA1_Stream0,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->LISR,
				&DMA1->LIFCR,
				0,
				DMA1_Stream0_IRQn
		},
		{
				DMA1_Stream1,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->LISR,
				&DMA1->LIFCR,
				6,
				DMA1_Stream1_IRQn
		},
		{
				DMA1_Stream2,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->LISR,
				&DMA1->LIFCR,
				16,
				DMA1_Stream2_IRQn
		},
		{
				DMA1_Stream3,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->LISR,
				&DMA1->LIFCR,
				22,
				DMA1_Stream3_IRQn
		},
		{
				DMA1_Stream4,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->HISR,
				&DMA1->HIFCR,
				0,
				DMA1_Stream4_IRQn
		},
		{
				DMA1_Stream5,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->HISR,
				&DMA1->HIFCR,
				6,
				DMA1_Stream5_IRQn
		},
		{
				DMA1_Stream6,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->HISR,
				&DMA1->HIFCR,
				16,
				DMA1_Stream6_IRQn
		},
		{
				DMA1_Stream7,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA1->HISR,
				&DMA1->HIFCR,
				22,
				DMA1_Stream7_IRQn
		},
		{
				DMA2_Stream0,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA1EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA1LPEN,
				&DMA2->LISR,
				&DMA2->LIFCR,
				0,
				DMA2_Stream0_IRQn
		},
		{
				DMA2_Stream1,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->LISR,
				&DMA2->LIFCR,
				6,
				DMA2_Stream1_IRQn
		},
		{
				DMA2_Stream2,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->LISR,
				&DMA2->LIFCR,
				16,
				DMA2_Stream2_IRQn
		},
		{
				DMA2_Stream3,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->LISR,
				&DMA2->LIFCR,
				22,
				DMA2_Stream3_IRQn
		},
		{
				DMA2_Stream4,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->HISR,
				&DMA2->HIFCR,
				0,
				DMA2_Stream4_IRQn
		},
		{
				DMA2_Stream5,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->HISR,
				&DMA2->HIFCR,
				6,
				DMA2_Stream5_IRQn
		},
		{
				DMA2_Stream6,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->HISR,
				&DMA2->HIFCR,
				16,
				DMA2_Stream6_IRQn
		},
		{
				DMA2_Stream7,
				NULL,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_DMA2EN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_DMA2LPEN,
				&DMA2->HISR,
				&DMA2->HIFCR,
				22,
				DMA2_Stream7_IRQn
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
				&RCC->APB2LPENR,
				RCC_APB2LPENR_USART1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_USART1RST,
				USART1_IRQn
		},
		{
				USART2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_USART2EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_USART2LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_USART2RST,
				USART2_IRQn
		},
		{
				USART3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_USART3EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_USART3LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_USART3RST,
				USART3_IRQn
		},
		{
				UART4,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_UART4EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_UART4LPEN,
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
				&RCC->APB1LPENR,
				RCC_APB1LPENR_TIM2LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM2RST,
				4,
				32,
				0,
				TIM2_IRQn
		},
		{
				TIM3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM3EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_TIM3LPEN,
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
				&RCC->APB1LPENR,
				RCC_APB1LPENR_TIM4LPEN,
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
				&RCC->APB1LPENR,
				RCC_APB1LPENR_TIM5LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM5RST,
				4,
				32,
				0,
				TIM5_IRQn
		},
		{
				TIM9,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_TIM9EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_TIM9LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_TIM9RST,
				2,
				16,
				0,
				TIM1_BRK_TIM9_IRQn
		},
		{
				TIM10,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_TIM10EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_TIM10LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_TIM10RST,
				1,
				16,
				0,
				TIM1_UP_TIM10_IRQn
		},
		{
				TIM11,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_TIM11EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_TIM11LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_TIM11RST,
				1,
				16,
				0,
				TIM1_TRG_COM_TIM11_IRQn
		},
		{
				TIM12,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM12EN,
				&RCC->APB2LPENR,
				RCC_APB1LPENR_TIM12LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM12RST,
				2,
				16,
				0,
				TIM8_BRK_TIM12_IRQn
		},
		{
				TIM13,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM13EN,
				&RCC->APB2LPENR,
				RCC_APB1LPENR_TIM13LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM13RST,
				1,
				16,
				0,
				TIM8_UP_TIM13_IRQn
		},
		{
				TIM14,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM14EN,
				&RCC->APB2LPENR,
				RCC_APB1LPENR_TIM14LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM14RST,
				1,
				16,
				0,
				TIM8_TRG_COM_TIM14_IRQn
		}
};

__TCH_STATIC_INIT tch_spi_descriptor SPI_HWs[MFEATURE_SPI] = {
		{
				SPI1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_SPI1EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_SPI1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_SPI1RST,
				SPI1_IRQn
		},
		{
				SPI2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_SPI2EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_SPI2LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_SPI2RST,
				SPI2_IRQn
		},
		{
				SPI3,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_SPI3EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_SPI3LPEN,
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
				&RCC->APB1LPENR,
				RCC_APB1LPENR_I2C1LPEN,
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
				&RCC->APB1LPENR,
				RCC_APB1LPENR_I2C2LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_I2C2RST,
				I2C2_EV_IRQn
		},
		{
				I2C3,
				NULL,
				0,
				&RCC->APB1ENR,
				RCC_APB1ENR_I2C3EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_I2C3LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_I2C3RST,
				I2C3_EV_IRQn
		}
};



__TCH_STATIC_INIT tch_adc_descriptor ADC_HWs[MFEATURE_ADC] = {
		{
				ADC1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC1EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_ADC1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADCRST,
				ADC_IRQn
		},
		{
				ADC2,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC2EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_ADC2PEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADCRST,
				ADC_IRQn
		},
		{
				ADC3,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_ADC3EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_ADC3LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_ADCRST,
				ADC_IRQn
		}
};


void tch_hal_enableSystick(){
	SysTick_Config(SYS_CLK / 1000);
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
	tch_kernel_onSystick();
}
