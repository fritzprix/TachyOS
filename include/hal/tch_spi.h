/*
 * tch_spi.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_SPI_H_
#define TCH_SPI_H_

#include "tch.h"

typedef uint8_t spi_t;
typedef struct tch_spi_handle_t tch_spiHandle;
struct _tch_spi_str_t {
	spi_t spi0;
	spi_t spi1;
	spi_t spi2;
	spi_t spi3;
	spi_t spi4;
	spi_t spi5;
	spi_t spi6;
	spi_t spi7;
	spi_t spi8;
};

struct _tch_spi_frame_format_t {
	uint8_t Frame16B;
	uint8_t Frame8B;
};

struct _tch_spi_frame_orient_t {
	uint8_t MSBFirst;
	uint8_t LSBFirst;
};

struct _tch_spi_opmode_t {
	uint8_t Master;
	uint8_t Slave;
};

struct _tch_spi_clkmode_t {
	uint8_t Mode0;
	uint8_t Mode1;
	uint8_t Mode2;
	uint8_t Mode3;
};

struct _tch_spi_baudrate_t {
	uint8_t High;
	uint8_t Normal;
	uint8_t Low;
};

typedef struct tch_spi_cfg_t {
	uint8_t ClkMode;
	uint8_t FrmFormat;
	uint8_t FrmOrient;
	uint8_t OpMode;
	uint8_t Baudrate;
}tch_spiCfg;

struct tch_spi_handle_t {
	tchStatus (*write)(tch_spiHandle* self,const void* wb,size_t sz, uint32_t timeout);
	tchStatus (*read)(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout);
	tchStatus (*transceive)(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
	tchStatus (*close)(tch_spiHandle* self);
};

typedef struct tch_lld_spi {
	struct _tch_spi_str_t spi;
	struct _tch_spi_clkmode_t ClkMode;
	struct _tch_spi_frame_format_t FrmFormat;
	struct _tch_spi_frame_orient_t FrmOrient;
	struct _tch_spi_opmode_t OpMode;
	struct _tch_spi_baudrate_t Baudrate;
	void (*initCfg)(tch_spiCfg* cfg);
	tch_spiHandle* (*open)(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
}tch_lld_spi;

extern const  tch_lld_spi* tch_spi_instance;


#endif /* TCH_SPI_H_ */
