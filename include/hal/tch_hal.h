/*
 * tch_hal.h
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

#ifndef TCH_HAL_H_
#define TCH_HAL_H_

#if defined(__cplusplus)
extern "C"{
#endif

#include "tch_halcfg.h"
#include "tch_haldesc.h"

typedef enum {
	LP_LEVEL0 = ((uint8_t) 0),
	LP_LEVEL1 = ((uint8_t) 1),
	LP_LEVEL2 = ((uint8_t) 2)
}tch_lplvl;

#include "tch_usart.h"
#include "tch_spi.h"
#include "tch_i2c.h"
#include "tch_adc.h"
#include "tch_rtc.h"
#include "tch_timer.h"
#include "tch_gpio.h"
#include "tch_dma.h"



extern __TCH_STATIC_INIT tch_gpio_descriptor GPIO_HWs[MFEATURE_GPIO];
extern __TCH_STATIC_INIT tch_ioInterrupt_descriptor IoInterrupt_HWs[MFEATURE_PINCOUNT_pPORT];
extern __TCH_STATIC_INIT tch_dma_descriptor DMA_HWs[MFEATURE_DMA];
extern __TCH_STATIC_INIT tch_uart_descriptor UART_HWs[MFEATURE_UART];
extern __TCH_STATIC_INIT tch_timer_descriptor TIMER_HWs[MFEATURE_TIMER];
extern __TCH_STATIC_INIT tch_spi_descriptor SPI_HWs[MFEATURE_SPI];
extern __TCH_STATIC_INIT tch_iic_descriptor IIC_HWs[MFEATURE_IIC];
extern __TCH_STATIC_INIT tch_adc_descriptor ADC_HWs[MFEATURE_ADC];



extern __TCH_STATIC_INIT tch_uart_bs UART_BD_CFGs[MFEATURE_GPIO];
extern __TCH_STATIC_INIT tch_timer_bs TIMER_BD_CFGs[MFEATURE_TIMER];
extern __TCH_STATIC_INIT tch_spi_bs SPI_BD_CFGs[MFEATURE_SPI];
extern __TCH_STATIC_INIT tch_iic_bs IIC_BD_CFGs[MFEATURE_IIC];
extern __TCH_STATIC_INIT tch_adc_bs ADC_BD_CFGs[MFEATURE_ADC];
extern __TCH_STATIC_INIT tch_adc_com_bs ADC_COM_BD_CFGs;


struct tch_hal_t{
	const tch_lld_usart* usart;
	const tch_lld_spi*   spi;
	const tch_lld_iic*   i2c;
	const tch_lld_adc*   adc;
	const tch_lld_gpio*  gpio;
	const tch_lld_timer* timer;
};

extern const tch_lld_rtc* tch_rtc;
extern const tch_lld_dma* tch_dma;

extern void tch_hal_enableSystick();
extern void tch_hal_disableSystick();

extern void tch_hal_setSleepMode(tch_lplvl lplvl);
extern void tch_hal_enterSleepMode();
extern void tch_hal_suspendSysClock();
extern void tch_hal_resumeSysClock();


#if defined(__cplusplus)
}
#endif
#endif /* TCH_HAL_H_ */
