/*
 * tch_i2c.c
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/STM_CMx/tch_hal.h"


typedef struct tch_lld_i2c_prototype {
	tch_lld_i2c                           pix;
} tch_lld_i2c_prototype;



__attribute__((section(".data"))) static tch_lld_i2c_prototype I2C_StaticInstance = {

};


const tch_lld_i2c* tch_i2c_instance = (const tch_lld_i2c*) &I2C_StaticInstance;


