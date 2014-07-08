/*
 * tch_adc.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
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
