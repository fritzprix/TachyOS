/*
 * tch_sdio.h
 *
 *  Created on: 2015. 1. 27.
 *      Author: innocentevil
 */

#ifndef TCH_SDIO_H_
#define TCH_SDIO_H_

#include "kernel/tch_ktypes.h"


typedef struct tch_sdio_handle* tch_sdioHandle_t;
typedef void* tch_sdioDevId;
typedef struct tch_sdio_device_info tch_sdioDevInfo;
typedef struct tch_sdio_cfg tch_sdioCfg_t;

typedef enum {
	MMC , SDC, SDIOC
}tch_sdioDevType;


struct tch_sdio_device_info {
	tch_sdioDevType			type;
	uint16_t 				id;
	BOOL 					is_protected;
	BOOL					is_locked;
};


struct tch_sdio_cfg {
	uint8_t bus_width;
	tch_PwrOpt lpopt;
};


struct tch_sdio_handle {
	uint32_t (*deviceId)(tch_sdioHandle_t sdio,tch_sdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
	tchStatus (*deviceInfo)(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info);
	tchStatus (*writeBlock)(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*readBlock)(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*erase)(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*eraseForce)(tch_sdioHandle_t sdio, tch_sdioDevId device);
	tchStatus (*setPassword)(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock);
	tchStatus (*lock)(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd);
	tchStatus (*unlock)(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd);
};


typedef struct tch_hal_module_sdio {
	void (*initCfg)(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt);
	tch_sdioHandle_t (*alloc)(const tch_core_api_t* api,const tch_sdioCfg_t* config,uint32_t timeout);
	tchStatus (*release)(tch_sdioHandle_t sdio);
}tch_hal_module_sdio_t;

#endif /* TCH_SDIO_H_ */
