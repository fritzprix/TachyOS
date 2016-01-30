/*
 * tch_sdio.c
 *
 *  Created on: 2016. 1. 27.
 *      Author: innocentevil
 */


#include "tch_sdio.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_dbg.h"
#include "tch_board.h"
#include "tch_board.h"

#ifndef SDIO_CLASS_KEY
#define SDIO_CLASS_KEY		(uint16_t) 0x3F65
#endif


#define SDIO_VALIDATE(sdio)		((struct tch_sdio_handle_prototype*) sdio)->status |= (0xffff & ((uint32_t) sdio ^ SDIO_CLASS_KEY))
#define SDIO_ISVALID(sdio)		(((struct tch_sdio_handle_prototype*) sdio)->status & 0xffff == (0xffff & (uint32_t) sdio ^ SDIO_CLASS_KEY))
#define SDIO_INVALIDATE(sdio)	((struct tch_sdio_handle_prototype*) sdio)->status &= ~0xffff

#define MAX_BUS_WIDTH		((uint8_t) 4)

#define SDIO_D0_PIN			((uint8_t) 8)
#define SDIO_D1_PIN			((uint8_t) 9)
#define SDIO_D2_PIN			((uint8_t) 10)
#define SDIO_D3_PIN			((uint8_t) 11)
#define SDIO_CK_PIN			((uint8_t) 12)

#define SDIO_CMD_PIN		((uint8_t) 2)

#define SDIO_DATA_PORT		tch_gpio2
#define SDIO_CMD_PORT		tch_gpio3

struct tch_sdio_handle_prototype  {
	struct tch_sdio_handle 				pix;
	uint32_t 							status;
	tch_gpioHandle*						data_port;
	tch_gpioHandle*						cmd_port;
	tch_dmaHandle*						dma;
	tch_eventId							evId;
	const tch_core_api_t*				env;
	tch_mtxId							mtx;
	tch_condvId							condv;
};

static int tch_sdio_init();
static void tch_sdio_exit();

__USER_API__ static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt);
__USER_API__ static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api,const tch_sdioCfg_t* config,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_release(tch_sdioHandle_t sdio);


__USER_API__ static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,tch_sdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
__USER_API__ static tchStatus tch_sdio_handle_deviceInfo(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info);
__USER_API__ static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,char* blk_bp,uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_sz,uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock);
__USER_API__ static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd);
__USER_API__ static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd);

__USER_RODATA__ tch_hal_module_sdio_t SDIO_Ops = {
		.initCfg = tch_sdio_initCfg,
		.alloc = tch_sdio_alloc,
		.release = tch_sdio_release
};


static tch_mtxCb mtx;
static tch_condvCb condv;

static tch_hal_module_dma_t* dma;
static tch_hal_module_gpio_t* gpio;


static int tch_sdio_init()
{
	dma = NULL;
	gpio = NULL;
	tch_mutexInit(&mtx);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_SDIO, SDIO_CLASS_KEY, &SDIO_Ops, FALSE);
}

static void tch_sdio_exit()
{
	tch_kmod_unregister(MODULE_TYPE_SDIO, SDIO_CLASS_KEY);
	tch_condvDeinit(&condv);
	tch_mutexDeinit(&mtx);
}


MODULE_INIT(tch_sdio_init);
MODULE_EXIT(tch_sdio_exit);


static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt)
{
	cfg->bus_width = buswidth;
	cfg->lpopt = lpopt;
}


static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api, const tch_sdioCfg_t* config,uint32_t timeout)
{
	if(!config)
		return NULL;

	struct tch_sdio_handle_prototype* ins = NULL;

	if(api->Mtx->lock(&mtx,timeout) != tchOK)
		goto RESOURCE_FAIL;
	while(SDIO_HWs[0]._handle != NULL)
	{
		if(api->Condv->wait(&condv,&mtx,timeout) != tchOK)
		{
			DBG_PRINT("SDIO Allocation Timeout\n\r");
			goto RESOURCE_FAIL;
		}
	}
	ins = (struct tch_sdio_handle_prototype*) api->Mem->alloc(sizeof(struct tch_sdio_handle_prototype));

	if(!ins)
	{
		DBG_PRINT("Out of Memory\n\r");
		goto RESOURCE_FAIL;
	}

	mset(ins,0, sizeof(struct tch_sdio_handle_prototype));

	dma = api->Module->request(MODULE_TYPE_DMA);
	if(!dma && SDIO_BD_CFGs[0].dma != DMA_NOT_USED)
	{
		DBG_PRINT("No DMA available");
		goto RESOURCE_FAIL;
	}

	gpio = api->Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		DBG_PRINT("GPIO is not Available");
		goto RESOURCE_FAIL;
	}

	// reclaim gpio required for SDIO operation
	gpio_config_t ioconfig;
	gpio->initCfg(&ioconfig);
	ioconfig.Mode = GPIO_Mode_AF;
	ioconfig.Otype = GPIO_Otype_PP;
	ioconfig.popt = config->lpopt;
	ioconfig.Speed = GPIO_OSpeed_100M;
	ioconfig.Af = SDIO_BD_CFGs[0].afv;
	ins->data_port = gpio->allocIo(api, SDIO_DATA_PORT, (1 << SDIO_D0_PIN | 1 << SDIO_D1_PIN |
			                                             1 << SDIO_D2_PIN | 1 << SDIO_D3_PIN | 
			                                             1 << SDIO_CK_PIN),
														&ioconfig, timeout);

	ins->cmd_port = gpio->allocIo(api, SDIO_CMD_PORT, 1 << SDIO_CMD_PIN, &ioconfig, timeout);


	if(!ins->data_port || !ins->cmd_port)
	{
		DBG_PRINT("GPIO(s) are not available for SDIO");
		goto RESOURCE_FAIL;
	}



	if(SDIO_BD_CFGs[0].dma != DMA_NOT_USED)
	{
		tch_DmaCfg dmacfg;
		dma->initCfg(&dmacfg);
		dmacfg.BufferType = DMA_BufferMode_Normal;
		dmacfg.Ch = SDIO_BD_CFGs[0].ch;
		dmacfg.Dir = DMA_Dir_MemToPeriph;
		dmacfg.FlowCtrl = DMA_FlowControl_Periph;
		dmacfg.Priority = DMA_Prior_High;
		dmacfg.mAlign = DMA_DataAlign_Word;
		dmacfg.mBurstSize = DMA_Burst_Inc4;
		dmacfg.mInc = TRUE;
		dmacfg.pAlign = DMA_DataAlign_Word;
		dmacfg.pBurstSize = DMA_Burst_Inc4;
		dmacfg.pInc = FALSE;
		ins->dma = dma->allocate(api, SDIO_BD_CFGs[0].dma, &dmacfg, timeout, config->lpopt);

		if(!ins->dma)
		{
//			DBG_PRINT("DMA is not abilable");
			goto RESOURCE_FAIL;
		}
	}


	api->Mtx->unlock(&mtx);


	// set ops for SDIO handle

	ins->pix.deviceId = tch_sdio_handle_device_id;
	ins->pix.deviceInfo = tch_sdio_handle_deviceInfo;
	ins->pix.erase = tch_sdio_handle_erase;
	ins->pix.eraseForce = tch_sdio_handle_eraseForce;
	ins->pix.lock = tch_sdio_handle_lock;
	ins->pix.unlock = tch_sdio_handle_unlock;
	ins->pix.readBlock = tch_sdio_handle_readBlock;
	ins->pix.writeBlock = tch_sdio_handle_writeBlock;
	ins->pix.setPassword = tch_sdio_handle_setPassword;

	ins->mtx = api->Mtx->create();
	ins->condv = api->Condv->create();

	if(!ins->mtx || !ins->condv)
	{
		api->Mtx->destroy(ins->mtx);
		api->Condv->destroy(ins->condv);
	}

	SDIO_VALIDATE(ins);

	return (tch_sdioHandle_t) ins;

RESOURCE_FAIL:
	if(ins)
	{
		if(ins->mtx)
			api->Mtx->destroy(ins->mtx);
		if(ins->condv)
			api->Condv->destroy(ins->condv);
		if(ins->cmd_port)
			ins->cmd_port->close(ins->cmd_port);
		if(ins->data_port)
			ins->data_port->close(ins->data_port);

		api->Mem->free(ins);
	}

	SDIO_HWs[0]._handle = NULL;
	api->Condv->wakeAll(&condv);
	api->Mtx->unlock(&mtx);
	return NULL;
}


static tchStatus tch_sdio_release(tch_sdioHandle_t sdio)
{

}


static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,tch_sdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt)
{

}

static tchStatus tch_sdio_handle_deviceInfo(tch_sdioHandle_t sdio,tch_sdioDevId device, tch_sdioDevInfo* info)
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
