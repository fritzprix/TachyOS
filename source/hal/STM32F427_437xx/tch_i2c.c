/*
 * tch_i2c.c
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


#include "hal/tch_hal.h"


typedef struct tch_lld_i2c_prototype {
	tch_lld_i2c                           pix;
} tch_lld_i2c_prototype;



__attribute__((section(".data"))) static tch_lld_i2c_prototype I2C_StaticInstance = {

};


const tch_lld_i2c* tch_i2c_instance = (const tch_lld_i2c*) &I2C_StaticInstance;


