/*
 * fmo_spi.h
 *
 *  Created on: 2014. 5. 8.
 *      Author: innocentevil
 */

#ifndef FMO_SPI_H_
#define FMO_SPI_H_

#include "../core/port/tch_stdtypes.h"
#include "hw_descriptor_types.h"


#define SPI_Baudrate_VeryHigh             ((uint8_t) 0)
#define SPI_Baudrate_High                 ((uint8_t) 1)
#define SPI_Baudrate_Mid                  ((uint8_t) 3)
#define SPI_Baudrate_Low                  ((uint8_t) 5)
#define SPI_Baudrate_VeryLow              ((uint8_t) 7)

#define SPI_ClockMode_0                   ((uint8_t) 0)
#define SPI_ClockMode_1                   ((uint8_t) 1)
#define SPI_ClockMode_2                   ((uint8_t) 2)
#define SPI_ClockMode_3                   ((uint8_t) 3)

#define SPI_DataFormat_8B                 ((uint8_t) 0)
#define SPI_DataFormat_16B                ((uint8_t) 1)

#define SPI_DataOrientation_MSBFIRST      ((uint8_t) 0)
#define SPI_DataOrientation_LSBFIRST      ((uint8_t) 1)

#define SPI_OPMode_Slave                  ((uint8_t) 0)
#define SPI_OPMode_Master                 ((uint8_t) 1)

typedef struct _tch_spi_cfg_t tch_spi_cfg;
typedef struct _tch_spi_instance_t tch_spi_instance;
typedef BOOL (*tch_spi_eventlistener)(tch_spi_instance* self,uint16_t rdata);

struct _tch_spi_cfg_t {
	uint8_t SPI_Baudrate;
	uint8_t SPI_ClockMode;
	uint8_t SPI_DataFormat;
	uint8_t SPI_DataOrientation;
	uint8_t SPI_OPMode;
};

struct _tch_spi_instance_t {
	BOOL (*open)(const tch_spi_instance* self,tch_spi_cfg* cfg,tch_pwrMgrCfg pcfg);
	uint16_t (*transceive)(const tch_spi_instance* self,uint16_t data,uint16_t* err);
	BOOL (*transceiveBurst)(const tch_spi_instance* self,const void* wb,void* rb,uint32_t size,uint16_t* err);
	BOOL (*read)(const tch_spi_instance* self,void* rb,uint32_t size,uint16_t* err);
	BOOL (*write)(const tch_spi_instance* self,const void* wb,uint32_t size,uint16_t* err);
	BOOL (*registerSlaveEventListener)(const tch_spi_instance* self,tch_spi_eventlistener listener);
	BOOL (*unregisterSlaveEventListener)(const tch_spi_instance* self);
	BOOL (*close)(const tch_spi_instance* self);
};

void tch_lld_spi_cfginit(tch_spi_cfg* cfg);

extern const tch_spi_instance* spi1;
extern const tch_spi_instance* spi2;
extern const tch_spi_instance* spi3;

#endif /* FMO_SPI_H_ */
