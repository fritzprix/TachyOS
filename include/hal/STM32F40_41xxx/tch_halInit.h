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
				GPIOF,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOFEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOFLPEN,
				0
		},
		{
				GPIOG,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOGEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOGLPEN,
				0
		},
		{
				GPIOH,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOHEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOHLPEN,
				0
		},
		{
				GPIOI,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOIEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOILPEN,
				0
		},
		{
				GPIOJ,
				&RCC->AHB1ENR,
				RCC_AHB1ENR_GPIOJEN,
				&RCC->AHB1LPENR,
				RCC_AHB1LPENR_GPIOJLPEN,
				0
		}
};
/*
 * 	void*                     io_occp;
	tch_condvId               condv;
	tch_barId                 evbar;
	IRQn_Type                 irq;
 */
__attribute__((section(".data"))) static tch_ioInterrupt_descriptor IoInterrupt_HWs[] = {
		{
				0,
				NULL,
				NULL,
				EXTI0_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI1_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI2_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI3_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI4_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				NULL,
				EXTI15_10_IRQn
		}
};


/**
 * 	void*               _hw;
	uint32_t*           _clkenr;
	const uint32_t       clkmsk;
	uint32_t*           _lpclkenr;
	const uint32_t       lpcklmsk;
	uint32_t*           _isr;
	uint32_t*           _icr;
	uint32_t             ipos;
	IRQn_Type            irq;
 */
__attribute__((section(".data"))) static tch_dma_descriptor DMA_HWs[] ={
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
 * 	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _isr;
	volatile uint32_t*  _icr;
	IRQn_Type            irq;
 */
__attribute__((section(".data"))) static tch_uart_descriptor UART_HWs[MFEATURE_UART] = {
		{
				USART1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_USART1EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_USART1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_USART1RST,
				&USART1->SR,
				&USART1->SR,
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
				&USART2->SR,
				&USART2->SR,
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
				&USART3->SR,
				&USART3->SR,
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
				&UART4->SR,
				&UART4->SR,
				UART4_IRQn
		}
};


/**
 * 		void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*   rstr;
	const uint32_t       rstmsk;
	volatile uint16_t*  _isr;
	volatile uint16_t*  _icr;
	const uint8_t        channelCnt;
	const uint8_t        precision;
	uint8_t              ch_occp;
	IRQn_Type            irq;
 * */
__attribute__((section(".data"))) static tch_timer_descriptor TIMER_HWs[MFEATURE_TIMER] = {
		{
				TIM2,
				NULL,
				&RCC->APB1ENR,
				RCC_APB1ENR_TIM2EN,
				&RCC->APB1LPENR,
				RCC_APB1LPENR_TIM2LPEN,
				&RCC->APB1RSTR,
				RCC_APB1RSTR_TIM2RST,
				&TIM2->SR,
				&TIM2->SR,
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
				&TIM3->SR,
				&TIM3->SR,
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
				&TIM4->SR,
				&TIM4->SR,
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
				&TIM5->SR,
				&TIM5->SR,
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
				&TIM9->SR,
				&TIM9->SR,
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
				&TIM10->SR,
				&TIM10->SR,
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
				&TIM11->SR,
				&TIM11->SR,
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
				&TIM12->SR,
				&TIM12->SR,
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
				&TIM13->SR,
				&TIM13->SR,
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
				&TIM14->SR,
				&TIM14->SR,
				1,
				16,
				0,
				TIM8_TRG_COM_TIM14_IRQn
		}
};

/**
 * 	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _rstr;
	const uint32_t       rstmsk;
	volatile uint16_t*  _isr;
	volatile uint16_t*  _icr;
	IRQn_Type            irq;
 */
__attribute__((section(".data"))) static tch_spi_descriptor SPI_HWs[MFEATURE_SPI] = {
		{
				SPI1,
				NULL,
				&RCC->APB2ENR,
				RCC_APB2ENR_SPI1EN,
				&RCC->APB2LPENR,
				RCC_APB2LPENR_SPI1LPEN,
				&RCC->APB2RSTR,
				RCC_APB2RSTR_SPI1RST,
				&SPI1->SR,
				&SPI1->SR,
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
				&SPI2->SR,
				&SPI2->SR,
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
				&SPI3->SR,
				&SPI3->SR,
				SPI3_IRQn
		}
};


#endif /* TCH_HALOBJS_H_ */
