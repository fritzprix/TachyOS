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




#define __TCH_STATIC_INIT  __attribute__((section(".data")))

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
	int8_t         txp;
	int8_t         rxp;
	int8_t         ctsp;
	int8_t         rtsp;
	uint8_t        afv;
}tch_uart_bs;


typedef struct _tch_timer_bs_t {
	gpIo_x         port;
	int8_t         chp[4];
	uint8_t        afv;
}tch_timer_bs;

typedef struct _tch_spi_bs_t {
	dma_t          txdma;
	dma_t          rxdma;
	uint8_t        txch;
	uint8_t        rxch;
	gpIo_x         port;
	int8_t         mosi;
	int8_t         miso;
	int8_t         sck;
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


#include "stm32f2xx.h"


#endif /* TCH_HALCFG_H_ */
