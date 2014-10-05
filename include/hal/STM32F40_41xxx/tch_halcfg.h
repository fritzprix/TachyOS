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
#define SYS_MCLK_FREQ              8000000    ///default system main clock  : 8MHz
#endif



#ifndef SYS_CLK
#define SYS_CLK                   ((uint32_t) 168000000)
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
#define MFEATURE_GPIO               (6)                     ///  define number of gpio port your platform
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


/*
 * 	{
				DMA2_Str7,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				GPIO_A,
				9,
				10
		},
		{
				DMA1_Str6,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				GPIO_A,
				2,
				3
		},
		{
				DMA1_Str4,
				DMA_NOT_USED,
				DMA_Ch7,
				DMA_Ch4,
				GPIO_B,
				10,
				11
		}
 */

__attribute__((section(".data"))) static tch_uart_bs UART_BD_CFGs[MFEATURE_GPIO] = {
		{
				DMA_Str15,
				DMA_NOT_USED,
				DMA_Ch4,
				DMA_Ch4,
				_GPIO_0,
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
				_GPIO_0,
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
				_GPIO_1,
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
				_GPIO_0,
				0,
				1,
				-1,
				-1,
				8
		}
};

#include "stm32f4xx.h"



#endif /* TCH_HALCFG_H_ */
