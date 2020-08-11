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



__TCH_STATIC_INIT tch_gpio_descriptor GPIO_HWs[MFEATURE_GPIO] = {
		{
				GPIOA,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOAEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOALPEN,
				0
		},
		{
				GPIOB,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOBEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOBLPEN,
				0
		},
		{
				GPIOC,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOCEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOCLPEN,
				0
		},
		{
				GPIOD,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIODEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIODLPEN,
				0
		},
		{
				GPIOE,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOEEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOELPEN,
				0
		},
		{
				GPIOH,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOHEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOHLPEN,
				0
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
				USART6,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_USART6EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_USART6LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_USART6RST,
				USART6_IRQn
		}
};


/**
 * typedef struct _tch_timer_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*   rstr;
	const uint32_t       rstmsk;
	const uint8_t        channelCnt;
	const uint8_t        precision;
	uint8_t              ch_occp;
	IRQn_Type            irq;
}tch_timer_descriptor;
 * */
__TCH_STATIC_INIT tch_timer_descriptor TIMER_HWs[MFEATURE_TIMER] = {
		{
				TIM1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_TIM1EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_TIM1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_TIM1RST,
				4,
				16,
				0,
				TIM1_CC_IRQn
		},
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
				32,
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
		},
		{
				SPI4,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_SPI4EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_SPI4LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_SPI4RST,
				SPI4_IRQn
		},
		{
				SPI5,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_SPI5EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_SPI5LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_SPI5RST,
				SPI5_IRQn
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
		}
};


__TCH_STATIC_INIT tch_sdio_descriptor SDIO_HWs[MFEATURE_SDIO] = {
		{
				._sdio = SDIO,
				._handle = NULL,
				._clkenr = &RCC->APB2ENR,
				.clkmsk = RCC_APB2ENR_SDIOEN,
				._lpclkenr = &RCC->APB2LPENR,
				.lpclkmsk = RCC_APB2LPENR_SDIOLPEN,
				._rstr = &RCC->APB2RSTR,
				.rstmsk = RCC_APB2RSTR_SDIORST,
				.irq = SDIO_IRQn
		}
};


/**
 *  implementation of HAL interface on which kernel depends
 */
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
