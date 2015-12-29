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


#ifndef TCH_HALCFG_H_
#define TCH_HALCFG_H_

#include "stm32f2xx.h"

#if defined(__cplusplus)
extern "C" {
#endif


///////////////////////////////////////////�� Configuration Constant Declarations��/////////////////////////////////////////////
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



///////////////////////////////////////////�� Configuration Block ��/////////////////////////////////////////////


/**
 *  @brief Use of Extrenal Memory
 *  if External Memory is used set this to '1', otherwise, '0'
 *
 *  @see EXTMEM_TYPE_SDRAM
 *  @see EXTMEM_TYPE_SRAM
 */
#ifndef USE_EXMEM
#define USE_EXMEM                0
#endif

#if (USE_EXMEM == 1)

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



#ifndef MFEATURE_GPIO
#define MFEATURE_GPIO               (6)          ///  define number of gpio port your platform
#endif

#ifndef MFEATURE_PINCOUNT_pPORT
#define MFEATURE_PINCOUNT_pPORT     (16)
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

#ifndef MFEATURE_SDIO
#define MFEATURE_SDIO				(1)
#endif



#if defined(__cplusplus)
}
#endif


#endif /* TCH_HALCFG_H_ */
