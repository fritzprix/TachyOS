/*
 * bl_config.h
 *
 *  Created on: 2014. 4. 21.
 *      Author: innocentevil
 */

#ifndef BL_CONFIG_H_
#define BL_CONFIG_H_

#include "fmo_dma.h"
#include "fmo_gpio.h"
#include "fmo_timer.h"

typedef struct _tch_bdc_usart_t tch_bdc_usart;
typedef struct _tch_bdc_adc_t tch_bdc_adc;

struct _tch_bdc_usart_t {
	dma_t               tx_dma;
	dma_t               rx_dma;
	uint8_t             tx_dma_ch;
	uint8_t             rx_dma_ch;
	tch_gpio_t           port;
	uint8_t             tx_pin;
	uint8_t             rx_pin;
	uint16_t            io_afv;
};

struct _tch_bdc_adc_t {
	dma_t                adc_dma;
	uint8_t              adc_dma_ch;
	tch_gpio_t           port;
	tch_timer            timer;
	tch_timer_event_type tevt;
	uint8_t              adc_pin;
	uint8_t              adc_ch;
};








__attribute__((section(".data")))static tch_bdc_usart USART_BD_CFGs[] = {
		{
				DMA2_Str7,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				GPIO_A,
				9,
				10,
				GPIO_AF_USART1
		},
		{
				DMA1_Str6,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				GPIO_A,
				2,
				3,
				GPIO_AF_USART2
		},
		{
				DMA1_Str3,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				GPIO_B,
				10,
				11,
				GPIO_AF_USART3
		}
};


/**
	dma_t                adc_dma;
	uint8_t              adc_dma_ch;
	tch_gpio_t           port;
	tch_timer            timer;
	tch_timer_event_type tevt;
	uint8_t              adc_pin;
	uint8_t              adc_ch;
 */
__attribute__((section(".data"))) static tch_bdc_adc ADC_BD_CFGs[] = {
		{
				DMA2_Str0,
				DMA_Ch0,
				GPIO_A,
				Timer2,
				CC2,
				4,
				4
		},
		{
				DMA2_Str2,
				DMA_Ch1,
				GPIO_A,
				Timer3,
				CC1,
				5,
				5
		},
		{
				DMA2_Str1,
				DMA_Ch2,
				GPIO_A,
				Timer5,
				CC1,
				3,
				3
		}
};



#endif /* BL_CONFIG_H_ */
