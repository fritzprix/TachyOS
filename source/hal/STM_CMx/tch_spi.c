/*
 * tch_spi.c
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/STM_CMx/tch_hal.h"


typedef struct tch_lld_spi_prototype {
	tch_lld_spi                          pix;
}tch_lld_spi_prototype;


__attribute__((section(".data"))) static tch_lld_spi_prototype SPI_StaticInstance;

const tch_lld_spi* tch_spi_instance = (const tch_lld_spi*) &SPI_StaticInstance;
