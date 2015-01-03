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

#include "tch_hal.h"
#include "tch_kernel.h"



__TCH_STATIC_INIT tch_gpio_descriptor GPIO_HWs[] = {
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
		}
};


__TCH_STATIC_INIT tch_ioInterrupt_descriptor IoInterrupt_HWs[] = {
		{
				0,
				NULL,
				EXTI0_IRQn
		},
		{
				0,
				NULL,
				EXTI1_IRQn
		},
		{
				0,
				NULL,
				EXTI2_IRQn
		},
		{
				0,
				NULL,
				EXTI3_IRQn
		},
		{
				0,
				NULL,
				EXTI4_IRQn
		},
		{
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				EXTI9_5_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
		},
		{
				0,
				NULL,
				EXTI15_10_IRQn
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




__TCH_STATIC_INIT tch_uart_bs UART_BD_CFGs[MFEATURE_GPIO] = {
		{
				DMA_Str15,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				tch_gpio0,
				9,
				10,
				11,
				12,
				7
		},
		{
				DMA_Str6,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				tch_gpio0,
				2,
				3,
				0,
				1,
				7
		},
		{
				DMA_Str4,
				DMA_NOT_USED,
				DMA_Ch7,
				DMA_Ch4,
				tch_gpio1,
				10,
				11,
				13,
				14,
				7
		},
		{
				DMA_Str4,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				tch_gpio0,
				0,
				1,
				-1,
				-1,
				8
		}
};

/**
 * 	gpIo_x         port;
	uint8_t        ch1p;
	uint8_t        ch2p;
	uint8_t        ch3p;
	uint8_t        ch4p;
	uint8_t        afv;
 */

__TCH_STATIC_INIT tch_timer_bs TIMER_BD_CFGs[MFEATURE_TIMER] = {
		{// TIM2
				tch_gpio0,
				{
						0,
						1,
						2,
						3
				},
				1
		},
		{// TIM3
				tch_gpio1,
				{
						4,
						5,
						0,
						1
				},
				2
		},
		{// TIM4
				tch_gpio1,
				{
						6,
						7,
						8,
						9
				},
				2
		},
		{// TIM5
				tch_gpio7,
				{
						10,
						11,
						12,
						-1
				},
				2
		},
		{// TIM9
				tch_gpio4,
				{
						5,
						6,
						-1,
						-1
				},
				3
		},
		{// TIM10
				tch_gpio1,
				{
						8,
						-1,
						-1,
						-1
				},
				3
		},
		{// TIM11
				tch_gpio1,
				{
						9,
						-1,
						-1,
						-1
				},
				3
		},
		{// TIM12
				tch_gpio1,
				{
						14,
						15,
						-1,
						-1
				},
				9
		},
		{// TIM13
				tch_gpio5,
				{
						8,
						-1,
						-1,
						-1
				},
				9
		},
		{// TIM14
				tch_gpio5,
				{
						9,
						-1,
						-1,
						-1
				},
				9
		}
};

/**
 * 	spi_t          spi;
	dma_t          txdma;
	dma_t          rxdma;
	gpIo_x         port;
	uint8_t        mosi;
	uint8_t        miso;
	uint8_t        sck;
 */

__TCH_STATIC_INIT tch_spi_bs SPI_BD_CFGs[MFEATURE_SPI] = {
		{
				DMA_Str13,    //dma2_stream5
				DMA_Str10,    //dma2_stream2
				3,            //dma channel 3
				3,            //dma channel 3
				0,            // port A (0)
				7,            // pin  7
				6,            // pin  6
				5,            // pin  5
				5             // af5
		},
		{
				DMA_Str4,     //dma1_stream4
				DMA_Str3,     //dma1_stream3
				0,
				0,
				1,            // port B (1)
				15,           // pin  15
				14,           // pin  14
				13,           // pin  13
				5             // af5
		},
		{
				DMA_Str7,     //dma1_stream7
				DMA_Str2,     //dma1_stream2
				0,
				0,
				2,            // port C (2)
				12,           // pin  12
				11,           // pin  11
				10,           // pin  10
				6
		}
};
/**
 * 	dma_t         txdma;
	dma_t         rxdma;
	uint8_t       txch;
	uint8_t       rxch;
	gpIo_x        port;
	uint8_t       scl;
	uint8_t       sda;
	uint8_t       afv;
 */

__TCH_STATIC_INIT tch_iic_bs IIC_BD_CFGs[MFEATURE_IIC] = {
		{      // IIC 1
				DMA_Str6,    // dma1_stream 6
				DMA_Str5,    // dma1_stream 5
				1,           // dma channel 1
				1,           // dma channel 1
				1,           // port B(1)
				6,           // pin 6
				7,           // pin 7
				4            // afv
		},
		{
				DMA_Str7,   // dma1_stream 2
				DMA_Str2,   // dma1_stream 7
				7,          // dma channel 7
				7,          // dma channel 7
				1,          // port B(1)
				10,          // pin 10
				11,          // pin 11
				4           // afv
		},
		{
				DMA_Str2,   // dma1 stream 2
				DMA_Str4,   // dma1 stream 4
				3,          // dma channel 3
				3,          // dma channel 3
				7,          // port H(7)
				7,          // pin 7
				8,          // pin 8
				4           // afv
		}
};

/**
 * typedef struct _tch_adc_bs_t {
	dma_t         dma;
	uint8_t       dmach;
	uint8_t       afv;
}tch_adc_bs;

typedef struct _tch_adc_com_bs_t{
	tch_port ports[MFEATURE_ADC_Ch];
	uint32_t occp_status;
};
 */


__TCH_STATIC_INIT tch_adc_bs ADC_BD_CFGs[MFEATURE_ADC] = {
		{
				DMA_Str12,
				DMA_Ch0,
				tch_TIMER0,  // TIM2
				2,         // CH2
				3      // Timer 2 CC2 Event
		},
		{
				DMA_Str10,
				DMA_Ch1,
				tch_TIMER1,
				1,
				7     // Timer 3 CC1 Event
		},
		{
				DMA_Str8,
				DMA_Ch0,
				tch_TIMER2,
				4,
				9     // Timer 4 CC4 Event
		}
};

#define ADC1_Bit            ((uint8_t) 1 << 0)
#define ADC2_Bit            ((uint8_t) 1 << 1)
#define ADC3_Bit            ((uint8_t) 1 << 2)

__TCH_STATIC_INIT tch_adc_com_bs ADC_COM_BD_CFGs = {
		{
				{tch_gpio0,0,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio0,1,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio0,2,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio0,3,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio0,3,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio0,4,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio0,5,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio0,6,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio0,7,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio1,0,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio1,1,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio2,0,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio2,1,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio2,2,(ADC1_Bit | ADC2_Bit | ADC3_Bit)},
				{tch_gpio2,4,(ADC1_Bit | ADC2_Bit)},
				{tch_gpio2,5,(ADC1_Bit | ADC2_Bit)}
		},
		0
};



tch_hal tch_hal_instance;
tch_lld_rtc* tch_rtc;
tch_lld_dma* tch_dma;

const tch_hal* tch_kernel_initHAL(const tch* env){
	/***
	 *  initialize clock subsystem
	 */

	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB1RSTR |= RCC_APB1RSTR_PWRRST;
	RCC->APB1RSTR &= ~RCC_APB1RSTR_PWRRST;

	tch_rtc = tch_rtcHalInit(env);
	tch_dma = tch_dmaHalInit(env);
	/***
	 *  bind hal interface
	 */
	tch_hal_instance.adc = tch_adcHalInit(env);
	tch_hal_instance.gpio  = tch_gpioHalInit(env);
	tch_hal_instance.timer = tch_timerHalInit(env);
	tch_hal_instance.usart = tch_usartHalInit(env);
	tch_hal_instance.spi = tch_spiHalInit(env);
	tch_hal_instance.i2c = tch_iicHalInit(env);

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
