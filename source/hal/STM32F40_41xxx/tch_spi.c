/*
 * tch_spi.c
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
#include "tch_spi.h"


typedef struct _tch_lld_spi_prototype tch_lld_spi_prototype;
typedef struct _tch_spi_handle_prototype tch_spi_handle_prototype;

#define TCH_SPI_CLASS_KEY            ((uint16_t) 0x41F5)
#define TCH_SPI_MASTER_FLAG          ((uint32_t) 0x10000)

#define SPI_FRM_FORMAT_16B           ((uint8_t) 1)
#define SPI_FRM_FORMAT_8B            ((uint8_t) 0)

#define SPI_FRM_ORI_MSBFIRST         ((uint8_t) 0)
#define SPI_FRM_ORI_LSBFIRST         ((uint8_t) 1)

#define SPI_OPMODE_MASTER            ((uint8_t) 0)
#define SPI_OPMODE_SLAVE             ((uint8_t) 1)

#define SPI_CLKMODE_0                ((uint8_t) 0)
#define SPI_CLKMODE_1                ((uint8_t) 1)
#define SPI_CLKMODE_2                ((uint8_t) 2)
#define SPI_CLKMODE_3                ((uint8_t) 3)

#define SPI_BAUDRATE_HIGH            ((uint8_t) 2)
#define SPI_BAUDRATE_NORMAL          ((uint8_t) 1)
#define SPI_BAUDRATE_LOW             ((uint8_t) 0)



#define INIT_SPI_STR                 {0,1,2,3,4,5,6,7,8}
#define INIT_SPI_FRM_FORMAT          {\
	                                   SPI_FRM_FORMAT_16B,\
	                                   SPI_FRM_FORMAT_8B\
}

#define INIT_SPI_FRM_ORIENT          {\
	                                   SPI_FRM_ORI_MSBFIRST,\
	                                   SPI_FRM_ORI_LSBFIRST\
}

#define INIT_SPI_OPMODE              {\
	                                   SPI_OPMODE_MASTER,\
	                                   SPI_OPMODE_SLAVE\
}

#define INIT_SPI_CLKMODE             {\
	                                   SPI_CLKMODE_0,\
	                                   SPI_CLKMODE_1,\
	                                   SPI_CLKMODE_2,\
	                                   SPI_CLKMODE_3\
}

#define INIT_SPI_BUADRATE            {\
                                       SPI_BAUDRATE_HIGH,\
                                       SPI_BAUDRATE_NORMAL,\
                                       SPI_BAUDRATE_LOW\
}


static void tch_spiInitCfg(tch_spiCfg* cfg);
static tch_spiHandle* tch_spiOpen(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout);

static tchStatus tch_spiWrite(tch_spiHandle* self,const void* wb,size_t sz, uint32_t timeout);
static tchStatus tch_spiRead(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout);
static tchStatus tch_spiTransceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout);
static tchStatus tch_spiClose(tch_spiHandle* self);


struct _tch_lld_spi_prototype {
	tch_lld_spi               pix;
	tch_mtxId                 mtx;
	tch_condvId               condv;
};

struct _tch_spi_handle_prototype {
	tch_spiHandle             pix;
	tch_DmaHandle*            tx_dma;
	tch_DmaHandle*            rx_dma;
	tch_GpioHandle*           iohandle;
	tch_mtxId                 mtx;
	tch_condvId               condv;
};

/**
 * 	struct _tch_spi_str_t spi;
	struct _tch_spi_clkmode_t ClkMode;
	struct _tch_spi_frame_format_t FrmFormat;
	struct _tch_spi_frame_orient_t FrmOrient;
	struct _tch_spi_opmode_t OpMode;
	struct _tch_spi_baudrate_t Baudrate;
	void (*initCfg)(tch_spiCfg* cfg);
	tch_spiHandle* (*open)(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout);
 */
__attribute__((section(".data"))) static tch_lld_spi_prototype SPI_StaticInstance = {
		{
			INIT_SPI_STR,
			INIT_SPI_CLKMODE,
			INIT_SPI_FRM_FORMAT,
			INIT_SPI_FRM_ORIENT,
			INIT_SPI_OPMODE,
			INIT_SPI_BUADRATE,
			tch_spiInitCfg,
			tch_spiOpen
		},
		NULL,
		NULL

};

const tch_lld_spi* tch_spi_instance = (const tch_lld_spi*) &SPI_StaticInstance;



static void tch_spiInitCfg(tch_spiCfg* cfg){

}

static tch_spiHandle* tch_spiOpen(tch* env,spi_t spi,tch_spiCfg* cfg,uint32_t timeout){

}

static tchStatus tch_spiWrite(tch_spiHandle* self,const void* wb,size_t sz, uint32_t timeout){

}

static tchStatus tch_spiRead(tch_spiHandle* self,void* rb,size_t sz, uint32_t timeout){

}

static tchStatus tch_spiTransceive(tch_spiHandle* self,const void* wb,void* rb,size_t sz,uint32_t timeout){

}

static tchStatus tch_spiClose(tch_spiHandle* self){

}
