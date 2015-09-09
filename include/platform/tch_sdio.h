/*
 * tch_sdio.h
 *
 *  Created on: 2015. 1. 27.
 *      Author: innocentevil
 */

#ifndef TCH_SDIO_H_
#define TCH_SDIO_H_


typedef struct tch_sdio_handle_t* tch_sdioHandle;
typedef struct tch_sdio_device_id_t tch_sdioDevId;
typedef struct tch_sdio_cfg_t tch_sdioCfg;

typedef enum {
	MMC , SDC, SDIOC
}tch_sdioDevType;

struct tch_sdio_device_id_t {

};

struct tch_sdio_cfg_t {

};

struct tch_sdio_handle_t {
	void (*close) (tch_sdioHandle sdio);
	tch_sdioDevId (*deviceId)(tch_sdioHandle sdio,tch_sdioDevType type);
	tchStatus (*writeBlock)(tch_sdioHandle sdio,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*readBlock)(tch_sdioHandle sdio,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*writeBurst)(tch_sdioHandle sdio,const char* bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t sz);
	tchStatus (*readBurst)(tch_sdioHandle sdio,char* bp,uint32_t blk_sz, uint32_t blk_offset,uint32_t sz);
	tchStatus (*erase)(tch_sdioHandle sdio,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
	tchStatus (*setProtection)(tch_sdioHandle sdio,BOOL protect,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
};

typedef struct tch_lld_sdio_t {
	void (*initCfg)();
	tch_sdioHandle (*allocSdio)();
}tch_lld_sdio;

#endif /* TCH_SDIO_H_ */
