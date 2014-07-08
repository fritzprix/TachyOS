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

#include "hal/STM_CMx/tch_hal.h"
#include "hal/STM_CMx/tch_halcfg.h"
#include "hal/STM_CMx/stm32f4xx.h"
#include "hal/STM_CMx/system_stm32f4xx.h"





tch_hal tch_hal_instance;

const tch_hal* tch_hal_init(){
	/***
	 *  initialize clock subsystem
	 */
	SystemInit();

	/***
	 *  bind hal interface
	 */
	tch_hal_instance.adc = tch_adc_instance;
	tch_hal_instance.gpio  = tch_gpio_instance;
	tch_hal_instance.timer = tch_timer_instance;
	tch_hal_instance.usart = tch_usart_instance;
	tch_hal_instance.spi = tch_spi_instance;
	tch_hal_instance.i2c = tch_i2c_instance;
	tch_hal_instance.rtc = tch_rtc_instance;


	return &tch_hal_instance;
}




