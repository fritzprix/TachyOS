/*
 * tch_boardcfg.c
 *
 *  Created on: Aug 23, 2015
 *      Author: innocentevil
 */

#include "tch_board.h"



__TCH_STATIC_INIT tch_uart_bs_t UART_BD_CFGs[MFEATURE_GPIO] = {
		{
				DMA_Str15,
//				DMA_NOT_USED,
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

__TCH_STATIC_INIT tch_timer_bs_t TIMER_BD_CFGs[MFEATURE_TIMER] = {
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

__TCH_STATIC_INIT tch_spi_bs_t SPI_BD_CFGs[MFEATURE_SPI] = {
		{

//				DMA_Str13,    //dma2_stream5
//				DMA_Str10,    //dma2_stream2
				DMA_NOT_USED,
				DMA_NOT_USED,
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

__TCH_STATIC_INIT tch_iic_bs_t IIC_BD_CFGs[MFEATURE_IIC] = {
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



__TCH_STATIC_INIT tch_adc_bs_t ADC_BD_CFGs[MFEATURE_ADC] = {
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

__TCH_STATIC_INIT tch_adc_channel_bs_t ADC_CH_BD_CFGs[MFEATURE_ADC_Ch] = {
		{{tch_gpio0,0},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch0
		{{tch_gpio0,1},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch1
		{{tch_gpio0,2},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch2
		{{tch_gpio0,3},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch3
		{{tch_gpio0,4},(ADC1_Bit | ADC2_Bit),0},// ch4
		{{tch_gpio0,5},(ADC1_Bit | ADC2_Bit),0},// ch5
		{{tch_gpio0,6},(ADC1_Bit | ADC2_Bit),0},// ch6
		{{tch_gpio0,7},(ADC1_Bit | ADC2_Bit),0},// ch7
		{{tch_gpio1,0},(ADC1_Bit | ADC2_Bit),0},// ch8
		{{tch_gpio1,1},(ADC1_Bit | ADC2_Bit),0},// ch9
		{{tch_gpio2,0},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch10
		{{tch_gpio2,1},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch11
		{{tch_gpio2,2},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch12
		{{tch_gpio2,3},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch13
		{{tch_gpio2,4},(ADC1_Bit | ADC2_Bit),0},
		{{tch_gpio2,5},(ADC1_Bit | ADC2_Bit),0}
};




