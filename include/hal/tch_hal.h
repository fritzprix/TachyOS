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


#if MFEATURE_UART
#include "tch_usart.h"
#endif

#if MFEATURE_SPI
#include "tch_spi.h"
#endif

#if MFEATURE_IIC
#include "tch_i2c.h"
#endif

#if MFEATURE_ADC
#include "tch_adc.h"
#endif



#include "tch_rtc.h"
#include "tch_timer.h"
#include "tch_gpio.h"
#include "tch_dma.h"


struct tch_hal_t{
#if MFEATURE_UART
	const tch_lld_usart* usart;
#endif

#if MFEATURE_SPI
	const tch_lld_spi*   spi;
#endif

#if MFEATURE_IIC
	const tch_lld_iic*   i2c;
#endif

#if MFEATURE_ADC
	const tch_lld_adc*   adc;
#endif

	const tch_lld_gpio*  gpio;
	const tch_lld_dma*   dma;
	const tch_lld_rtc*   rtc;
	const tch_lld_timer* timer;
};

extern void __tch_stderr(const void*,size_t);
extern void __tch_stdout(const void*,size_t);
extern void __tch_stdin(void*,size_t);


#if defined(__cplusplus)
}
#endif
#endif /* TCH_HAL_H_ */
