/*
 * tch_sdio.c
 *
 *  Created on: 2016. 1. 27.
 *      Author: innocentevil
 */


#include "tch_sdio.h"
#include "tch_hal.h"
#include "tch_board.h"
#include "tch_dma.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_dbg.h"



#ifndef SDIO_CLASS_KEY
#define SDIO_CLASS_KEY				(uint16_t) 0x3F65
#endif

#define MAX_SDIO_DEVCNT				10
#define CMD_CLEAR_MASK              ((uint32_t)0xFFFFF800)
#define SDIO_ACMD41_ARG_HCS			(1 << 30)
#define SDIO_ACMD41_ARG_XPC			(1 << 28)
#define SDIO_ACMD41_ARG_S18R		(1 << 24)

#define SDIO_R3_CMD					((1 << 6) - 1)

#define SDIO_R3_IDLE				((uint32_t) 1 << 31)
#define SDIO_R3_CCS					((uint32_t) 1 << 30)
#define SDIO_R3_UHSII				((uint32_t) 1 << 29)
#define SDIO_R3_S18A				((uint32_t) 1 << 24)

#define SDIO_MODE_DMA				((uint32_t) 0x10000)
#define SDIO_BUSY					((uint32_t) 0x20000)


#define SDIO_VALIDATE(ins)	do {\
	((struct tch_sdio_handle_prototype*) ins)->status |= (0xffff & ((uint32_t) ins ^ SDIO_CLASS_KEY));\
}while(0)

#define SDIO_INVALIDATE(ins) do {\
	((struct tch_sdio_handle_prototype*) ins)->status &= ~0xffff;\
}while(0)


#define SDIO_ISVALID(ins)			((((struct tch_sdio_handle_prototype*) ins)->status & 0xffff) == (((uint32_t) ins ^ SDIO_CLASS_KEY) & 0xffff))



#define SET_BUSY(sdio) do {\
	set_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status |= SDIO_BUSY;\
}while(0)

#define CLR_BUSY(sdio) do {\
	clear_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status &= ~SDIO_BUSY;\
}while(0)

#define IS_BUSY(sdio) 		((struct tch_sdio_handle_prototype*) sdio)->status & SDIO_BUSY

#define SET_DMA_MODE(sdio)		sdio->status |= SDIO_MODE_DMA
#define CLR_DMA_MODE(sdio)		sdio->status &= ~SDIO_MODE_DMA
#define IS_DMA_MODE(sdio)		(sdio->status & SDIO_MODE_DMA)

#define MAX_BUS_WIDTH		((uint8_t) 4)

#define SDIO_D0_PIN			((uint8_t) 8)
#define SDIO_D1_PIN			((uint8_t) 9)
#define SDIO_D2_PIN			((uint8_t) 10)
#define SDIO_D3_PIN			((uint8_t) 11)
#define SDIO_CK_PIN			((uint8_t) 12)

#define SDIO_CMD_PIN		((uint8_t) 2)

#define SDIO_DETECT_PIN		((uint8_t) 8)

#define SDIO_DETECT_PORT	tch_gpio0
#define SDIO_DATA_PORT		tch_gpio2
#define SDIO_CMD_PORT		tch_gpio3

struct tch_sdio_handle_prototype  {
	struct tch_sdio_handle 				pix;
	uint32_t 							status;
	tch_gpioHandle*						data_port;
	tch_gpioHandle*						cmd_port;
	tch_gpioHandle*						detect_port;
	tch_dmaHandle*						dma;
	tch_eventId							evId;
	const tch_core_api_t*				api;
	tch_mtxId							mtx;
	tch_condvId							condv;
};


struct tch_sdio_device_info {
	SdioDevType				type;
#define SDIO_CAP_SDSC					((uint8_t) 0)
#define SDIO_CAP_SDHXC					((uint8_t) 1)
	uint8_t					cap;
#define SDIO_S18A_SUPPORT_FLAG			((uint8_t) 1)
#define SDIO_UHSII_SUPPORT_FLAG			((uint8_t) 2)
#define SDIO_READY_FLAG					((uint8_t) 4)
	uint8_t					status;
	uint16_t 				id;
	BOOL 					is_protected;
	BOOL					is_locked;
};

typedef uint8_t		sdio_resp_t;
#define SDIO_RESP_SHORT		((sdio_resp_t) 1)
#define SDIO_RESP_LONG		((sdio_resp_t) 2)


/*** private function declaration ***/
static int tch_sdio_init();
static void tch_sdio_exit();


static uint32_t sdio_mmc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static tchStatus sdio_send_cmd(struct tch_sdio_handle_prototype* ins, uint8_t cmdidx, uint32_t arg, BOOL waitresp, sdio_resp_t rtype, uint32_t* resp,uint32_t timeout);


__USER_API__ static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt);
__USER_API__ static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api,const tch_sdioCfg_t* config,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_release(tch_sdioHandle_t sdio);


__USER_API__ static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
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

static tch_sdioDevInfo dev_infos[MAX_SDIO_DEVCNT];
static uint8_t devcnt;


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
	const tch_sdio_bs_t* sdio_bs = &SDIO_BD_CFGs[0];
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];

	if(api->Mtx->lock(&mtx,timeout) != tchOK)
		goto INIT_FAIL;
	while(sdio_hw->_handle != NULL)
	{
		if(api->Condv->wait(&condv,&mtx,timeout) != tchOK)
		{
			api->Dbg->print(api->Dbg->Normal, 0, "SDIO Allocation Timeout\n\r");
			goto INIT_FAIL;
		}
	}
	ins = (struct tch_sdio_handle_prototype*) api->Mem->alloc(sizeof(struct tch_sdio_handle_prototype));
	sdio_hw->_handle = ins;
	if(!ins)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "Out of Memory\n\r");
		goto INIT_FAIL;
	}

	mset(ins,0, sizeof(struct tch_sdio_handle_prototype));

	dma = tch_kmod_request(MODULE_TYPE_DMA);
	if(!dma && sdio_bs->dma != DMA_NOT_USED)
	{

		api->Dbg->print(api->Dbg->Normal, 0, "DMA is Not available\n\r");
		goto INIT_FAIL;
	}

	gpio = api->Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "GPIO is Not available\n\r");
		goto INIT_FAIL;
	}

	gpio_config_t ioconfig;
	gpio->initCfg(&ioconfig);
	ioconfig.Mode = GPIO_Mode_AF;
	ioconfig.Otype = GPIO_Otype_PP;
	ioconfig.popt = config->lpopt;
	ioconfig.Speed = GPIO_OSpeed_100M;
	ioconfig.Af = sdio_bs->afv;
	ins->data_port = gpio->allocIo(api, SDIO_DATA_PORT, (1 << SDIO_D0_PIN | 1 << SDIO_D1_PIN |
			                                             1 << SDIO_D2_PIN | 1 << SDIO_D3_PIN | 
			                                             1 << SDIO_CK_PIN),
														&ioconfig, timeout);

	ins->cmd_port = gpio->allocIo(api, SDIO_CMD_PORT, 1 << SDIO_CMD_PIN, &ioconfig, timeout);

	if(!ins->data_port || !ins->cmd_port)
	{
		api->Dbg->print(api->Dbg->Normal, 0, "GPIO(s) Not available\n\r");
		goto INIT_FAIL;
	}

	if(sdio_bs->dma != DMA_NOT_USED)
	{
		tch_DmaCfg dmacfg;
		dma->initCfg(&dmacfg);
		dmacfg.BufferType = DMA_BufferMode_Normal;
		dmacfg.Ch = sdio_bs->ch;
		dmacfg.Dir = DMA_Dir_MemToPeriph;
		dmacfg.FlowCtrl = DMA_FlowControl_Periph;
		dmacfg.Priority = DMA_Prior_High;
		dmacfg.mAlign = DMA_DataAlign_Word;
		dmacfg.mBurstSize = DMA_Burst_Inc4;
		dmacfg.mInc = TRUE;
		dmacfg.pAlign = DMA_DataAlign_Word;
		dmacfg.pBurstSize = DMA_Burst_Inc4;
		dmacfg.pInc = FALSE;
		ins->dma = dma->allocate(api, sdio_bs->dma, &dmacfg, timeout, config->lpopt);
		SET_DMA_MODE(ins);
		if(!ins->dma)
		{
			api->Dbg->print(api->Dbg->Normal, 0, "DMA is not abilable\n\r");
			goto INIT_FAIL;
		}
	}

	// set ops for SDIO handle
	mset(dev_infos, 0, sizeof(struct tch_sdio_device_info) * MAX_SDIO_DEVCNT);
	devcnt = 0;


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
	ins->evId = api->Event->create();

	if(!ins->mtx || !ins->condv || !ins->evId)
	{
		goto INIT_FAIL;
	}

	api->Mtx->unlock(&mtx);

	*sdio_hw->_clkenr |= sdio_hw->clkmsk;			// power main clock enable
	if(config->lpopt == ActOnSleep)
		*sdio_hw->_lpclkenr |= sdio_hw->lpclkmsk;	// if sdio should be operational in idle mode, enable SDIO lp clock

	*sdio_hw->_rstr |= sdio_hw->rstmsk;				// reset sdio
	*sdio_hw->_rstr &= ~sdio_hw->rstmsk;

	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	sdio_reg->POWER |= SDIO_POWER_PWRCTRL;			// power up

	// set interrupts
	if(!ins->dma)
	{
		// DMA support is not available, enable interrupt for data handling
		sdio_reg->MASK |= (SDIO_MASK_SDIOITIE | SDIO_MASK_RXDAVLIE);
	}
	else
	{
		sdio_reg->CLKCR |= SDIO_CLKCR_HWFC_EN;		// hardware flow control enable
	}
	// enable error handling interrupt
	sdio_reg->MASK |= (SDIO_MASK_RXOVERRIE | SDIO_MASK_TXUNDERRIE | SDIO_MASK_CMDRENDIE | SDIO_MASK_CMDSENTIE | SDIO_MASK_CCRCFAILIE);
	sdio_reg->CLKCR |= SDIO_CLKCR_PWRSAV;			// power save mode enabled
	sdio_reg->CLKCR |= (SDIO_CLKCR_CLKDIV & 198);	// set clock divisor 48MHz / 200  because should be less than 400kHz
	sdio_reg->CLKCR |= SDIO_CLKCR_CLKEN;			// set sdio clock enable
	sdio_reg->DCTRL |= SDIO_DCTRL_DTEN;

	// set clock register for Identification phase

	ins->api = api;
	tch_enableInterrupt(sdio_hw->irq, HANDLER_HIGH_PRIOR);
	SDIO_VALIDATE(ins);

	return (tch_sdioHandle_t) ins;

INIT_FAIL:
	if(ins)
	{
		if(ins->mtx)
			api->Mtx->destroy(ins->mtx);
		if(ins->condv)
			api->Condv->destroy(ins->condv);
		if(ins->evId)
			api->Event->destroy(ins->evId);
		if(ins->cmd_port)
			ins->cmd_port->close(ins->cmd_port);
		if(ins->data_port)
			ins->data_port->close(ins->data_port);

		api->Mem->free(ins);
	}

	sdio_hw->_handle = NULL;
	api->Condv->wakeAll(&condv);
	api->Mtx->unlock(&mtx);
	return NULL;
}


static tchStatus tch_sdio_release(tch_sdioHandle_t sdio)
{
	if(!sdio)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;

	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	const tch_core_api_t* api = ins->api;
	if(api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return tchErrorResource;
	while(IS_BUSY(sdio))
	{
		if(api->Condv->wait(ins->condv,ins->mtx, tchWaitForever) != tchOK)
			return tchErrorResource;
	}
	SET_BUSY(sdio);
	api->Mtx->unlock(ins->mtx);

	ins->cmd_port->close(ins->cmd_port);
	ins->data_port->close(ins->data_port);
	dma->freeDma(ins->dma);

	*sdio_hw->_rstr |= sdio_hw->rstmsk;
	*sdio_hw->_rstr &= ~sdio_hw->rstmsk;

	*sdio_hw->_lpclkenr &= ~sdio_hw->lpclkmsk;
	*sdio_hw->_clkenr &= ~sdio_hw->clkmsk;

	SDIO_INVALIDATE(ins);
	CLR_BUSY(ins);
	api->Mtx->destroy(ins->mtx);
	api->Condv->destroy(ins->condv);
	api->Event->destroy(ins->evId);
	api->Mem->free(ins);
	return tchOK;
}


static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt)
{
	if(!sdio)
		return 0;

	if(!SDIO_ISVALID(sdio))
		return 0;

	switch(type){
	case MMC:
		return sdio_mmc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
	case SDC:
		return sdio_sdc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
	case SDIOC:
		return sdio_sdioc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
	default:
		return 0;
	}
	return 0;
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

static uint32_t sdio_mmc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{

}

uint32_t loopcnt = 0;


static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{
	loopcnt = 0;
	if(!ins || !devIds)
		return 0;
	const tch_core_api_t* api = ins->api;
	if(api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return 0;

	while(IS_BUSY(ins))
	{
		if(api->Condv->wait(ins->condv, ins->mtx, tchWaitForever) != tchOK)
			return 0;
	}
	SET_BUSY(ins);
	if(api->Mtx->unlock(ins->mtx) != tchOK)
		return 0;

	tchStatus res;
	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;

	sdio_send_cmd(ins, 0, 0, FALSE, SDIO_RESP_SHORT, NULL, tchWaitForever);			// send reset

	uint32_t resp[4];
	sdio_send_cmd(ins, 8, 0x1FF, TRUE, SDIO_RESP_SHORT, resp, tchWaitForever);		// send op cond

	res = tchOK;

	while(res != tchErrorTimeoutResource && (devcnt < max_idcnt))
	{
		res = tchOK;
		sdio_send_cmd(ins, 55, 0, TRUE, SDIO_RESP_SHORT, resp, tchWaitForever);
		devcnt++;
		res = sdio_send_cmd(ins, 41, SDIO_ACMD41_ARG_HCS , TRUE, SDIO_RESP_SHORT, resp, tchWaitForever);

		loopcnt++;
		if(sdio_reg->RESPCMD == SDIO_R3_CMD)
		{
			if(!(resp[0] & SDIO_R3_IDLE))
			{
				mset(&dev_infos[devcnt], 0 ,sizeof(struct tch_sdio_device_info));			// clear device info
				dev_infos[devcnt].type = SDC;
				dev_infos[devcnt].cap = resp[0] >> 29;
				if(resp[0] >> 23)
					dev_infos[devcnt].status |= SDIO_S18A_SUPPORT_FLAG;
	//			api->Dbg->print(api->Dbg->Normal,0 , "SDIO Ready");

			}
		}
	}


	if(api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return 0;
	CLR_BUSY(ins);
	api->Condv->wakeAll(ins->condv);
	api->Mtx->unlock(ins->mtx);
}

static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{

}

static tchStatus sdio_send_cmd(struct tch_sdio_handle_prototype* ins, uint8_t cmdidx, uint32_t arg, BOOL waitresp,sdio_resp_t rtype, uint32_t* resp, uint32_t timeout)
{
	if(!ins)
		return tchErrorParameter;
	tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	int32_t ev;

	sdio_reg->ARG = arg;
	tchStatus res;
	uint32_t cmd = (SDIO_CMD_CMDINDEX & cmdidx) | SDIO_CMD_CPSMEN;
	if(waitresp)
	{
		cmd |= ((rtype == SDIO_RESP_SHORT)? SDIO_CMD_WAITRESP_0 : SDIO_CMD_WAITRESP);
		sdio_reg->CMD = cmd;
		if((res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDREND, timeout)) != tchOK)
			return res;
		ev = ins->api->Event->clear(ins->evId, SDIO_STA_CMDREND);
		mcpy(resp, &sdio_reg->RESP1, 4 * sizeof(uint32_t));
		if((ev & ~SDIO_STA_CMDREND))
		{
			return tchErrorIo;
		}
	}
	else
	{
		cmd |= SDIO_CMD_ENCMDCOMPL;
		sdio_reg->CMD = cmd;
		if((res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDSENT, timeout)) != tchOK)
			return res;
		ins->api->Event->clear(ins->evId, SDIO_STA_CMDSENT);
	}
	return res;
}


// interrupt handler
void SDIO_IRQHandler()
{
	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	struct tch_sdio_handle_prototype* sdio_ins = sdio_hw->_handle;
	const tch_core_api_t* api = sdio_ins->api;
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	if(sdio_reg->STA & SDIO_STA_CMDSENT)
	{
		sdio_reg->ICR |= SDIO_ICR_CMDSENTC;
		api->Event->set(sdio_ins->evId,SDIO_STA_CMDSENT);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Sent\n\r",(sdio_reg->CMD & SDIO_CMD_CMDINDEX));
		return;
	}
	if(sdio_reg->STA & SDIO_STA_CMDREND)
	{
		sdio_reg->ICR |= SDIO_ICR_CMDRENDC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CMDREND);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Responded\n\r",(sdio_reg->RESPCMD & SDIO_RESPCMD_RESPCMD));
		return;
	}
	if(sdio_reg->STA & SDIO_STA_CCRCFAIL)
	{
		sdio_reg->ICR |= SDIO_ICR_CCRCFAILC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CMDREND | SDIO_STA_CCRCFAIL);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Responded\n\r wiht CRC Error", (sdio_reg->RESPCMD & SDIO_RESPCMD_RESPCMD));
		return;
	}
}

