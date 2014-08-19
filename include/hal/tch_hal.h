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

#include "tch_halcfg.h"



#include "tch_usart.h"
#include "tch_spi.h"
#include "tch_i2c.h"
#include "tch_adc.h"
#include "tch_rtc.h"
#include "tch_timer.h"
#include "tch_intr.h"
#if MFEATURE_GPIO
#include "tch_gpio.h"
#endif

struct tch_hal_t{
#if MFEATURE_GPIO
	const tch_lld_gpio*  gpio;
#endif
	const tch_lld_timer* timer;
	const tch_lld_usart* usart;
	const tch_lld_spi*   spi;
	const tch_lld_i2c*   i2c;
	const tch_lld_adc*   adc;
	const tch_lld_rtc*   rtc;
	const tch_lld_intr*  interrupt;
};

extern void __tch_printError(tchStatus err);
extern void __tch_stdout(const void*,uint32_t);
extern void __tch_stdin(void*,uint32_t);

#endif /* TCH_HAL_H_ */
