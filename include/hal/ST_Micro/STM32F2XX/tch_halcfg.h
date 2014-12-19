/**
 * @file tch_halcfg.h
 * @brief this file contains Macro definition for Hal Configuration
 *
 * @author doowoong,lee
 * @bug    No Known Bug
 */


/*
 * tch_halcfg.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#include "tch_dma.h"
#include "tch_gpio.h"
#include "tch_timer.h"

#ifndef TCH_HALCFG_H_
#define TCH_HALCFG_H_

#define TACHYOS_HAL_VENDOR               "ST_Micro"                      ///< vendor name field
#define TACHYOS_HAL_PLATFORM_NAME        HW_PLF                          ///< hw platform name (product name of mcu / mpu ic)
#define TACHYOS_HAL_PLATFORM_SPECIFIER    1                              ///< hw platform specifier (mapped to sub product number ex > stm32f407zg , stm32f417...)
#define TACHYOS_HAL_MAIN_VERSION          0                              ///< hal version
#define TACHYOS_HAL_SUB_VERSION           0                              ///< hal sub version

///////////////////////////////////////////¡é Configuration Constant Declarations¡é/////////////////////////////////////////////
/**
 *  @brief Us External Crystal, so that internal inverter is enabled for generating main clock
 */
#define SYS_MCLK_TYPE_CRYSTAL     0

/**
 *  @brief Use External Osc. which has its own driver and High Q resonating node so internal crystal driver is by-passed
 */
#define SYS_MCLK_TYPE_EXTOSC      1

/**
 *  @brief Use Internal Osc. so there is no need for additional component (but usually show less quality than the others)
 */
#define SYS_MCLK_TYPE_INTERNAL    2


/**
 *  @brief External Memory Type : SDRAM
 *  @see USE_EXTMEM
 *  @see EXTMEM_TYPE
 */
#define EXTMEM_TYPE_SDRAM         0

/**
 * @brief External Memory Type : SRAM
 * @see USE_EXTMEM
 * @see EXTMEM_TYPE
 */
#define EXTMEM_TYPE_SRAM          1



///////////////////////////////////////////¡é Configuration Block ¡é/////////////////////////////////////////////


/**
 *  @brief Use of Extrenal Memory
 *  if External Memory is used set this to '1', otherwise, '0'
 *
 *  @see EXTMEM_TYPE_SDRAM
 *  @see EXTMEM_TYPE_SRAM
 */
#ifndef USE_EXTMEM
#define USE_EXTMEM                0
#endif

#if (USE_EXTMEM == 1)

#ifndef EXTMEM_TYPE
 #error "Please define 'EXTMEM_TYPE' "
#endif
#endif


#ifndef SYS_MCLK_TYPE
/**
 *  @brief configure clock source type of target system  : default external crystal
 */
#define SYS_MCLK_TYPE            SYS_MCLK_TYPE_CRYSTAL
#endif





#ifndef SYS_INTERNAL_CLK_FREQ
/**
 *  configure clock source frequency
 */
#define SYS_INTERNAL_CLK_FREQ      16000000
#endif


#ifndef SYS_MCLK_FREQ
#define SYS_MCLK_FREQ              25000000    ///default system main clock  : 25MHz
#endif



#ifndef SYS_CLK
#define SYS_CLK                   ((uint32_t) 120000000)
#endif

/****
 *    config system clock speed
 *     - Fastest  :  possible Highest Clock is set as system clock
 *                   typically it's more power consuming
 *     - fair     :  1/2 of highest clock
 *     - low      :  1/4 of highest clock
 */
#ifndef SYS_TARGET_CLK_SPEED
#define SYS_TARGET_CLK_SPEED       SYS_TARGET_CLK_FASTEST
#endif

#define SYS_SLEEPCLK_ENABLE         0
#define SYS_SLEEPCLK                0

#if (SYS_SLEEPCLK_ENABLE && (!SYS_SLEEPCLK))
#error "You should Define SYS_SLEEPCLK"
#endif



/***
 *  configure stm32f4x vendor library header macro
 */


#if (SYS_MCLK_TYPE != SYS_MCLK_TYPE_INTERNAL)
#define HSE_VALUE                SYS_MCLK_FREQ
#else
#define HSE_VALUE                SYS_INTERNAL_CLK_FREQ
#endif


#ifndef MFEATURE_GPIO
#define MFEATURE_GPIO               (5)          ///  define number of gpio port your platform
#endif

#ifndef MFEATURE_PINCOUNT_pPORT                            ///   define number of pin count per each gpio port
#define MFEATURE_PINCOUNT_pPort     (16)
#endif

#ifndef MFEATURE_DMA
#define MFEATURE_DMA                (16)
#endif

#ifndef MFEATURE_DMA_Ch
#define MFEATURE_DMA_Ch             (8)
#endif

#ifndef MFEATURE_UART
#define MFEATURE_UART               (4)
#endif

#ifndef MFEATURE_TIMER
#define MFEATURE_TIMER              (10)
#endif

#ifndef MFEATURE_SPI
#define MFEATURE_SPI                (3)
#endif

#ifndef MFEATURE_IIC
#define MFEATURE_IIC                (3)
#endif


#ifndef MFEATURE_ADC
#define MFEATURE_ADC                (3)
#endif


#ifndef MFEATURE_ADC_Ch
#define MFEATURE_ADC_Ch             (16)
#endif




typedef struct _tch_port_t {
	gpIo_x          port;
	uint8_t         pin;
}tch_port;

typedef struct _tch_uart_bs_t {
	dma_t          txdma;
	dma_t          rxdma;
	uint8_t        txch;
	uint8_t        rxch;
	gpIo_x         port;
	uint8_t        txp;
	uint8_t        rxp;
	uint8_t        ctsp;
	uint8_t        rtsp;
	uint8_t        afv;
}tch_uart_bs;


typedef struct _tch_timer_bs_t {
	gpIo_x         port;
	uint8_t        chp[4];
	uint8_t        afv;
}tch_timer_bs;

typedef struct _tch_spi_bs_t {
	dma_t          txdma;
	dma_t          rxdma;
	uint8_t        txch;
	uint8_t        rxch;
	gpIo_x         port;
	uint8_t        mosi;
	uint8_t        miso;
	uint8_t        sck;
	uint8_t        afv;
}tch_spi_bs;

typedef struct _tch_iic_bs_t {
	dma_t         txdma;
	dma_t         rxdma;
	uint8_t       txch;
	uint8_t       rxch;
	gpIo_x        port;
	uint8_t       scl;
	uint8_t       sda;
	uint8_t       afv;
}tch_iic_bs;

typedef struct _tch_adc_bs_t {
	dma_t         dma;
	uint8_t       dmaCh;
	tch_timer     timer;
	uint8_t       timerCh;
	uint8_t       timerExtsel;
}tch_adc_bs;

typedef struct _tch_adc_port_t {
	tch_port      port;
	uint8_t       adc_msk;
}tch_adc_port;

typedef struct _tch_adc_com_bs_t{
	tch_adc_port ports[MFEATURE_ADC_Ch];
	uint32_t occp_status;
}tch_adc_com_bs;


__attribute__((section(".data"))) static tch_uart_bs UART_BD_CFGs[MFEATURE_GPIO] = {
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
			//	DMA_NOT_USED,
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

__attribute__((section(".data"))) static tch_timer_bs TIMER_BD_CFGs[MFEATURE_TIMER] = {
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

__attribute__((section(".data"))) static tch_spi_bs SPI_BD_CFGs[MFEATURE_SPI] = {
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

__attribute__((section(".data"))) static tch_iic_bs IIC_BD_CFGs[MFEATURE_IIC] = {
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
				DMA_Str2,   // dma1_stream 2
				DMA_Str7,   // dma1_stream 7
				7,          // dma channel 7
				7,          // dma channel 7
				5,          // port B(1)
				0,          // pin 10
				1,          // pin 11
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


__attribute__((section(".data"))) static tch_adc_bs ADC_BD_CFGs[MFEATURE_ADC] = {
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

__attribute__((section(".data"))) static tch_adc_com_bs ADC_COM_BD_CFGs = {
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

#include "stm32f2xx.h"


#endif /* TCH_HALCFG_H_ */
