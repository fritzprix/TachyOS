/*
 * tch_gpio.c
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


#include "hal/tch_gpio.h"
#include "tch_haldesc.h"




typedef struct _tch_gpio_handle_internal_t {
	tch_gpio_handle              _pix;

}tch_gpio_prototype;

static tch_gpio_handle* tch_gpio_allocIo(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,tch_pwr_def pcfg);
static uint16_t tch_gpio_getPortCount();
static uint16_t tch_gpio_getPincount(const gpIo_x port);
static uint32_t tch_gpio_getPinAvailable(const gpIo_x port);
static void tch_gpio_freeIo(tch_gpio_handle* IoHandle);



const tch_lld_gpio* tch_gpio_instance = NULL;





tch_gpio_handle* tch_gpio_allocIo(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,tch_pwr_def pcfg){
	tch_gpio_descriptor* gpio = &GPIO_HWs[port];
	if(!gpio->_clkenr){                     /// given GPIO port is not supported in this H/W
		return NULL;
	}

}

uint16_t tch_gpio_getPortCount(){

}

uint16_t tch_gpio_getPincount(const gpIo_x port){

}

uint32_t tch_gpio_getPinAvailable(const gpIo_x port){

}

void tch_gpio_freeIo(tch_gpio_handle* IoHandle){

}
