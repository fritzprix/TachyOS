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
#include "tch_display.h"


typedef struct _tch_hal_t tch_hal;


typedef struct _tch_lld_usart_t tch_lld_usart;
typedef struct _tch_lld_spi_t tch_lld_spi;
typedef struct _tch_lld_i2c_t tch_lld_i2c;
typedef struct _tch_lld_adc_t tch_lld_adc;
typedef struct _tch_lld_gpio_t tch_lld_gpio;
typedef struct _tch_lld_gptimer_t tch_lld_gptimer;
typedef struct _tch_lld_pwm_t tch_lld_pwm;
typedef struct _tch_lld_rtc_t tch_lld_rtc;
typedef struct _tch_lld_dsc_t tch_lld_dsc;


struct _tch_hal_t {
	const tch_lld_gpio* gpio;
	const tch_lld_pwm* pwm;
	const tch_lld_gptimer* gpt;
	const tch_lld_usart* usart;
	const tch_lld_spi* spi;
	const tch_lld_i2c* i2c;
	const tch_lld_adc* adc;
	const tch_lld_rtc* rtc;
	const tch_lld_dsc* dsc;
};



const tch_hal* tch_hal_init();



#endif /* TCH_HAL_H_ */
