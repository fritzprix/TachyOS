/*
 * tch_hal.c
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#include "tch.h"
#include "hal/STM_CMx/tch_hal.h"
#include "hal/STM_CMx/tch_halcfg.h"
#include "hal/STM_CMx/stm32f4xx.h"
#include "hal/STM_CMx/system_stm32f4xx.h"

#include "hal/STM_CMx/tch_gpio.h"
#include "hal/STM_CMx/tch_usart.h"
#include "hal/STM_CMx/tch_spi.h"
#include "hal/STM_CMx/tch_i2c.h"
#include "hal/STM_CMx/tch_rtc.h"
#include "hal/STM_CMx/tch_adc.h"
#include "hal/STM_CMx/tch_display.h"
#include "hal/STM_CMx/tch_timer.h"




extern const tch_lld_gpio* _TCH_GPIO_Instance;
extern const tch_lld_pwm* _TCH_PWM_Instance;
extern const tch_lld_gptimer* _TCH_GPTIMER_Instance;
extern const tch_lld_usart* _TCH_USART_Instance;
extern const tch_lld_spi* _TCH_SPI_Instance;
extern const tch_lld_i2c* _TCH_I2C_Instance;
extern const tch_lld_adc* _TCH_ADC_Instance;
extern const tch_lld_rtc* _TCH_RTC_Instance;
extern const tch_lld_dsc* _TCH_DSC_Instance;

tch_hal TCH_HAL_OBJECT;

const tch_hal* tch_hal_init(){
	/***
	 *  initialize clock subsystem
	 */
	SystemInit();

	/***
	 *  bind hal interface
	 */
	TCH_HAL_OBJECT.adc = _TCH_ADC_Instance;
	TCH_HAL_OBJECT.gpio  = _TCH_GPIO_Instance;
	TCH_HAL_OBJECT.usart = _TCH_USART_Instance;
	TCH_HAL_OBJECT.spi = _TCH_SPI_Instance;
	TCH_HAL_OBJECT.i2c = _TCH_I2C_Instance;
	TCH_HAL_OBJECT.gpt = _TCH_GPTIMER_Instance;
	TCH_HAL_OBJECT.pwm = _TCH_PWM_Instance;
	TCH_HAL_OBJECT.rtc = _TCH_RTC_Instance;
	TCH_HAL_OBJECT.dsc = _TCH_DSC_Instance;


	return &TCH_HAL_OBJECT;
}




