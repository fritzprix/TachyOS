/*
 * tch_halobjs.h
 *
 *  Created on: 2014. 7. 20.
 *      Author: innocentevil
 */

#ifndef TCH_HALOBJS_H_
#define TCH_HALOBJS_H_

#include "stm32f4xx.h"
#include "tch_haldesc.h"

__attribute__((section(".data"))) static tch_gpio_descriptor GPIO_HWs[] = {
		{
				GPIOA,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOAEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOALPEN,
				0
		},
		{
				GPIOB,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOBEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOBLPEN,
				0
		},
		{
				GPIOC,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOCEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOCLPEN,
				0
		},
		{
				GPIOD,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIODEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIODLPEN,
				0
		},
		{
				GPIOE,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOEEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOELPEN,
				0
		},
		{
				GPIOF,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOFEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOFLPEN,
				0
		},
		{
				GPIOG,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOGEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOGLPEN,
				0
		},
		{
				GPIOH,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOHEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOHLPEN,
				0
		},
		{
				GPIOI,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOIEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOILPEN,
				0
		},
		{
				GPIOJ,
				(uint32_t*)&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOJEN,
				(uint32_t*)&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOJLPEN,
				0
		}
};

__attribute__((section(".data"))) static tch_ioInterrupt_descriptor IoInterrupt_HWs[] = {
		{
				0,
				EXTI0_IRQn
		},
		{
				0,
				EXTI1_IRQn
		},
		{
				0,
				EXTI2_IRQn
		},
		{
				0,
				EXTI3_IRQn
		},
		{
				0,
				EXTI4_IRQn
		},
		{
				0,
				EXTI9_5_IRQn
		},
		{
				0,
				EXTI9_5_IRQn
		},
		{
				0,
				EXTI9_5_IRQn
		},
		{
				0,
				EXTI9_5_IRQn
		},
		{
				0,
				EXTI9_5_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		},
		{
				0,
				EXTI15_10_IRQn
		}
};



#endif /* TCH_HALOBJS_H_ */
