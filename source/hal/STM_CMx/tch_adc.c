/*
 * tch_adc.c
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/STM_CMx/tch_hal.h"



typedef struct tch_lld_adc_prototype {
	tch_lld_adc                          pix;
}tch_lld_adc_prototype;


static tch_lld_adc_prototype ADC_StaticInstance = {

};

const tch_lld_adc* tch_adc_instance = (tch_lld_adc*) &ADC_StaticInstance;
