/*
 * tch_hal.h
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_HAL_H_
#define TCH_HAL_H_


#include "hal/STM_CMx/stm32f4xx.h"
#include "tch_usart.h"
#include "tch_spi.h"
#include "tch_i2c.h"
#include "tch_adc.h"
#include "tch_rtc.h"
#include "tch_timer.h"
#include "tch_gpio.h"



typedef struct tch_hal{
	const tch_lld_gpio*  gpio;
	const tch_lld_timer* timer;
	const tch_lld_usart* usart;
	const tch_lld_spi*   spi;
	const tch_lld_i2c*   i2c;
	const tch_lld_adc*   adc;
	const tch_lld_rtc*   rtc;
}tch_hal;



const tch_hal* tch_hal_init();



#endif /* TCH_HAL_H_ */
