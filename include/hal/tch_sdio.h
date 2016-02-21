/*
 * tch_sdio.h
 *
 *  Created on: 2015. 1. 27.
 *      Author: innocentevil
 */

#ifndef TCH_SDIO_H_
#define TCH_SDIO_H_

#include "kernel/tch_ktypes.h"



#define SDIO_VO27_28			((uint16_t) 1)
#define SDIO_VO28_29			((uint16_t) 1 << 1)
#define SDIO_VO29_30			((uint16_t) 1 << 2)
#define SDIO_VO30_31			((uint16_t) 1 << 3)
#define SDIO_VO31_32			((uint16_t) 1 << 4)
#define SDIO_VO32_33			((uint16_t) 1 << 5)
#define SDIO_VO33_34			((uint16_t) 1 << 6)
#define SDIO_VO34_35			((uint16_t) 1 << 7)
#define SDIO_VO35_36			((uint16_t) 1 << 8)

#define SDIO_PREPARE_OPT_SPEED		((uint8_t) 1)
#define SDIO_PREPARE_OPT_MAX_COMPAT	((uint8_t) 2)

typedef struct tch_sdio_handle* tch_sdioHandle_t;
typedef void* tch_sdioDevId;
typedef struct tch_sdio_cfg tch_sdioCfg_t;

typedef enum {
	MMC , SDC, SDIOC, UNKNOWN
}SdioDevType;

typedef struct tch_sdio_info {
	uint8_t 		ver;			// CSD Version   0 : SC  /  1 : HC XC
	uint32_t 		tr_spd;			// max speed in MHz
	uint8_t			rd_blen;		// block size for read
	uint8_t			wr_blen;		// block size for write
	BOOL			is_partial_rd_allowed;
	BOOL			is_partial_wr_allowed;
	BOOL			is_erase_blk_enable;
	BOOL			perm_wp;
	BOOL			tmp_wp;
	uint8_t			sect_size;
	uint8_t			file_fmt;
	uint64_t		capacity;
}__attribute__((packed)) tch_sdio_info_t;


struct tch_sdio_cfg {
	uint8_t 		bus_width;
	uint16_t 		v_opt;
	tch_PwrOpt 		lpopt;
	BOOL			s18r_enable;		// sd card specific?
}__attribute__((packed));


struct tch_sdio_handle {
	tchStatus (*deviceReset)(tch_sdioHandle_t sdio);
	uint32_t (*deviceId)(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
	tchStatus (*getDevInfo)(tch_sdioHandle_t sdio, tch_sdioDevId device, tch_sdio_info_t* info);
	tchStatus (*prepare)(tch_sdioHandle_t sdio,tch_sdioDevId device,uint8_t option);
	SdioDevType (*getDeviceType)(tch_sdioHandle_t sdio, tch_sdioDevId device);
	BOOL (*isProtectEnabled)(tch_sdioHandle_t sdio,tch_sdioDevId device);
	uint64_t  (*getMaxBitrate)(tch_sdioHandle_t sdio, tch_sdioDevId device);
	uint64_t  (*getCapacity)(tch_sdioHandle_t sdio, tch_sdioDevId device);
	tchStatus (*writeBlock)(tch_sdioHandle_t sdio,tch_sdioDevId device ,const uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout);
	tchStatus (*readBlock)(tch_sdioHandle_t sdio,tch_sdioDevId device,uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout);
	tchStatus (*erase)(tch_sdioHandle_t sdio,tch_sdioDevId device,uint32_t blk_offset,uint32_t blk_cnt);
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
