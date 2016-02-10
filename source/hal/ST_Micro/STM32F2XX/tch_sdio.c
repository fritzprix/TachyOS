/*
 * tch_sdio.c
 *
 *  Created on: 2016. 1. 27.
 *      Author: innocentevil
 */


#include "tch_sdio.h"
#include "kernel/tch_kernel.h"

#ifndef SDIO_CLASS_KEY
#define SDIO_CLASS_KEY		(uint16_t) 0x3F65
#endif


__USER_API__ static void tch_initCfg(tch_sdioCfg* cfg);
__USER_API__ static tch_sdioHandle_t tch_sdio_alloc(const tch_sdioCfg* config);
__USER_API__ static tchStatus tch_sdio_release(tch_sdioHandle_t sdio);


__USER_API__ static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,tch_sdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
__USER_API__ static tchStatus tch_sdio_deviceInfo(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info);
__USER_API__ static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock);
__USER_API__ static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd);
__USER_API__ static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd);





static void tch_initCfg(tch_sdioCfg* cfg)
{

}


static tch_sdioHandle_t tch_sdio_alloc(const tch_sdioCfg* config)
{

}


static tchStatus tch_sdio_release(tch_sdioHandle_t sdio)
{

}


static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,tch_sdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt)
{

}

static tchStatus tch_sdio_deviceInfo(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info)
{

}

static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt)
{

}

static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}

static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock)
{

}

static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd)
{

}

static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd)
{

}
