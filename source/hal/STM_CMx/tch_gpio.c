/*
 * tch_gpio.c
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/STM_CMx/tch_hal.h"


typedef struct tch_lld_gpio_prototype {
	tch_lld_gpio                   pix;
}tch_lld_gpio_prototype;


__attribute__((section(".data")))  static tch_lld_gpio GPIO_StaticInstance = {

};

const tch_lld_gpio* tch_gpio_instance = (tch_lld_gpio*) &GPIO_StaticInstance;
