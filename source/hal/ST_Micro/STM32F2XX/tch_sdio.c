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
#include "kernel/tch_interrupt.h"



#ifndef SDIO_CLASS_KEY
#define SDIO_CLASS_KEY					(uint16_t) 0x3F65
#endif


#define MAX_SDIO_DEVCNT					10
#define MAX_TIMEOUT_MILLS				1000
#define CMD_CLEAR_MASK					((uint32_t)0xFFFFF800)
#define SDIO_ACMD41_ARG_HCS				(1 << 30)
#define SDIO_ACMD41_ARG_XPC				(1 << 28)

#define SDIO_VO_TO_VW_SHIFT				15
#define SDIO_OCR_VW27_28				(1 << 15)
#define SDIO_OCR_VW28_29				(1 << 16)
#define SDIO_OCR_VW29_30				(1 << 17)
#define SDIO_OCR_VW30_31				(1 << 18)
#define SDIO_OCR_VW31_32				(1 << 19)
#define SDIO_OCR_VW32_33				(1 << 20)
#define SDIO_OCR_VW33_34				(1 << 21)
#define SDIO_OCR_VW34_35				(1 << 22)
#define SDIO_OCR_VW35_36				(1 << 23)

#define SDIO_R3_READY					((uint32_t) 1 << 31)
#define SDIO_R3_CCS						((uint32_t) 1 << 30)
#define SDIO_R3_UHSII					((uint32_t) 1 << 29)
#define SDIO_R3_S18A					((uint32_t) 1 << 24)

/*
 * SDIO CMD index List
 * */
#define CMD_GO_IDLE_STATE				((uint8_t) 0)
#define CMD_ALL_SEND_CID				((uint8_t) 2)
#define CMD_SEND_RCA					((uint8_t) 3)
#define CMD_SET_DSR						((uint8_t) 4)		// program the DSR of all cards
#define CMD_SELECT						((uint8_t) 7)		// command toggle a card between stand-by and transfer state
#define CMD_SEND_IF_COND				((uint8_t) 8)		// send interface condition
#define CMD_SEND_CSD					((uint8_t) 9)		// query card specific data(CSD)
#define CMD_SEND_CID					((uint8_t) 10)		// query card identification(CID)
#define CMD_SIGVL_SW					((uint8_t) 11)		// switch to 1.8V bus signal level
#define CMD_STOP_TRANS					((uint8_t) 12)		// forces the card to stop transmission
#define CMD_SEND_STATUS					((uint8_t) 13)		// addressed card send its status
#define CMD_GO_INACTIVE					((uint8_t) 15)		// make an addressed card into the inactive state

#define CMD_SET_BLKLEN					((uint8_t) 16)
#define CMD_READ_SBLK					((uint8_t) 17)
#define CMD_READ_MBLK					((uint8_t) 18)
#define CMD_SEND_TUNE					((uint8_t) 19)
#define CMD_SPD_CLS_CTRL				((uint8_t) 20)
#define CMD_SET_BLKCNT					((uint8_t) 23)
#define CMD_WRITE_SBLK					((uint8_t) 24)
#define CMD_WRITE_MBLK					((uint8_t) 25)
#define CMD_PROG_CSD					((uint8_t) 27)
#define CMD_SET_WP						((uint8_t) 28)
#define CMD_CLR_WP						((uint8_t) 29)
#define CMD_SEND_WP						((uint8_t) 30)
#define CMD_ERASE_WBLK_START			((uint8_t) 32)
#define CMD_ERASE_WBLK_END				((uint8_t) 33)
#define CMD_ERASE						((uint8_t) 38)
#define CMD_DEF_DPS						((uint8_t) 40)
#define CMD_LOCK_UNLOCK					((uint8_t) 42)
#define CMD_APP_CMD						((uint8_t) 55)
#define CMD_GEN_CMD						((uint8_t) 56)

#define ACMD_SET_BUS_WIDTH				((uint8_t) 6)
#define ACMD_SD_STATUS					((uint8_t) 13)
#define ACMD_SEND_NUM_WR_BLK			((uint8_t) 22)
#define ACMD_SEND_WR_BLK_ERASE_CNT		((uint8_t) 23)
#define ACMD_SEND_OP_COND				((uint8_t) 41)
#define ACMD_SET_CLR_CARD_DETECT		((uint8_t) 42)
#define ACMD_SEND_SCR					((uint8_t) 51)

/*
 * Flags for SDIO Handle
 * */
#define SDIO_HANDLE_FLAG_DMA			((uint32_t) 0x10000)
#define SDIO_HANDLE_FLAG_BUSY			((uint32_t) 0x20000)


/*
 * Flags of SD Card Status Field @ R1 response
 * */
#define SDC_STATUS_OOR					((uint32_t) 1 << 31)
#define SDC_STATUS_ADDRESS_ERR			((uint32_t) 1 << 30)
#define SDC_STATUS_BLEN_ERR				((uint32_t) 1 << 29)
#define SDC_STATUS_ERASE_SEQ_ERR		((uint32_t) 1 << 28)
#define SDC_STATUS_ERASE_PARM_ERR		((uint32_t) 1 << 27)
#define SDC_STATUS_WP_VIOLATION_ERR		((uint32_t) 1 << 26)
#define SDC_STATUS_CARD_IS_LOCKED		((uint32_t) 1 << 25)
#define SDC_STATUS_LOCK_FAIL			((uint32_t) 1 << 24)
#define SDC_STATUS_CMDCRC_ERR			((uint32_t) 1 << 23)
#define SDC_STATUS_ILLCMD_ERR			((uint32_t) 1 << 22)
#define SDC_STATUS_CARDECC_ERR			((uint32_t) 1 << 21)
#define SDC_STATUS_CC_ERR				((uint32_t) 1 << 20)
#define SDC_STATUS_ERR					((uint32_t) 1 << 19)
#define SDC_STATUS_CSD_OVW_ERR			((uint32_t) 1 << 16)
#define SDC_STATUS_WP_ERASE_SKIP		((uint32_t) 1 << 15)
#define SDC_STATUS_CARD_ECC_DIS			((uint32_t) 1 << 14)
#define SDC_STATUS_ERASE_RESET			((uint32_t) 1 << 13)
#define SDC_STATUS_CUR_STATE_MSK		((uint32_t) 0xF << 9)
#define SDC_STATUS_DATA_RDY				((uint32_t) 1 << 8)
#define SDC_STATUS_APP_CMD				((uint32_t) 1 << 5)
#define SDC_STATUS_AKE_SEQ_ERR			((uint32_t) 1 << 3)


#define SCR_DAT_BUS_WIDTH				((uint32_t) 0xF << 16)
#define SCR_SPEC_VER					((uint32_t) 0xF << 24)

#define BUS_WIDTH_1B					((uint8_t) 1)
#define BUS_WIDTH_4B					((uint8_t) 1 << 2)
#define MAX_BUS_WIDTH					((uint8_t) 4)

#define SDIO_D0_PIN						((uint8_t) 8)
#define SDIO_D1_PIN						((uint8_t) 9)
#define SDIO_D2_PIN						((uint8_t) 10)
#define SDIO_D3_PIN						((uint8_t) 11)
#define SDIO_CK_PIN						((uint8_t) 12)

#define SDIO_CMD_PIN					((uint8_t) 2)

#define SDIO_DETECT_PIN					((uint8_t) 8)

#define SDIO_DETECT_PORT				tch_gpio0
#define SDIO_DATA_PORT					tch_gpio2
#define SDIO_CMD_PORT					tch_gpio3


#define CSD_VERSION						((uint32_t) 3 << 30)
#define CSDv1_ACC_TIME					((uint32_t) 0xFF << 16)
#define CSDv1_NSAC						((uint32_t) 0xFF << 8)
#define CSDv1_TRANS_SPEED				((uint32_t) 0xFF << 0)

#define CSDv1_CMD_CLASS					((uint32_t) ((1 << 12) - 1) << 20)
#define CSDv1_RD_BLKLEN					((uint32_t) 0xF << 16)
#define CSDv1_RD_PART_OK				((uint32_t) 1 << 15)
#define CSDv1_WR_BLK_MIS				((uint32_t) 1 << 14)
#define CSDv1_RD_BLK_MIS				((uint32_t) 1 << 13)
#define CSDv1_DSR_IMPL					((uint32_t) 1 << 12)
#define CSDv1_CSIZE_UPPER				((uint32_t) ((1 << 10) - 1))

#define CSDv1_CSIZE_LOWER				((uint32_t) 3 << 30)
#define CSDv1_VDD_RD_MIN				((uint32_t) 7 << 27)
#define CSDv1_VDD_RD_MAX				((uint32_t) 7 << 24)
#define CSDv1_VDD_WR_MIN				((uint32_t) 3 << 21)
#define CSDv1_VDD_WR_MAX				((uint32_t) 3 << 18)
#define CSDv1_CSIZE_MUL					((uint32_t) 7 << 15)
#define CSDv1_ERASE_BLK_EN				((uint32_t) 1 << 14)
#define CSDv1_SECT_SIZE					((uint32_t) 0x7F << 7)
#define CSDv1_WP_GRP_SIZE				((uint32_t) 0x7F)

#define CSDv1_WP_GRP_EN					((uint32_t) 1 << 31)
#define CSDv1_R2W_FACTOR				((uint32_t) 7 << 26)
#define CSDv1_WR_BLKLEN					((uint32_t) 0xF << 22)
#define CSDv1_WR_PART_OK				((uint32_t) 1 << 21)
#define CSDv1_FILE_FMT_GRP				((uint32_t) 1 << 15)
#define CSDv1_COPY_FLAG					((uint32_t) 1 << 14)
#define CSDv1_PERM_WP					((uint32_t) 1 << 13)
#define CSDv1_TMP_WP					((uint32_t) 1 << 12)
#define CSDv1_FILE_FMT					((uint32_t) 3 << 10)


#define CSDv2_ACC_TIME					((uint32_t) 0xFF << 16)
#define CSDv2_NSAC						((uint32_t) 0xFF << 8)
#define CSDv2_TRANS_SPEED				((uint32_t) 0xFF << 0)

#define CSDv2_CMD_CLASS					((uint32_t) ((1 << 12) - 1) << 20)
#define CSDv2_RD_BLKLEN					((uint32_t) 0xF << 16)
#define CSDv2_RD_PART_OK				((uint32_t) 1 << 15)
#define CSDv2_WR_BLK_MIS				((uint32_t) 1 << 14)
#define CSDv2_RD_BLK_MIS				((uint32_t) 1 << 13)
#define CSDv2_DSR_IMPL					((uint32_t) 1 << 12)
#define CSDv2_CSIZE_UPPER				((uint32_t) ((1 << 6) - 1))

#define CSDv2_CSIZE_LOWER				((uint32_t) (((1 << 16) - 1) << 16))
#define CSDv2_ERASE_BLK_EN				((uint32_t) 1 << 14)
#define CSDv2_SECT_SIZE					((uint32_t) 0x7F << 7)
#define CSDv2_WP_GRP_SIZE				((uint32_t) 0x7F)

#define CSDv2_WP_GRP_EN					((uint32_t) 1 << 31)
#define CSDv2_R2W_FACTOR				((uint32_t) 7 << 26)
#define CSDv2_WR_BLKLEN					((uint32_t) 0xF << 22)
#define CSDv2_WR_PART_OK				((uint32_t) 1 << 21)
#define CSDv2_FILE_FMT_GRP				((uint32_t) 1 << 15)
#define CSDv2_COPY_FLAG					((uint32_t) 1 << 14)
#define CSDv2_PERM_WP					((uint32_t) 1 << 13)
#define CSDv2_TMP_WP					((uint32_t) 1 << 12)
#define CSDv2_FILE_FMT					((uint32_t) 3 << 10)


#define SDIO_VALIDATE(ins)	do {\
	((struct tch_sdio_handle_prototype*) ins)->status |= (0xffff & ((uint32_t) ins ^ SDIO_CLASS_KEY));\
}while(0)

#define SDIO_INVALIDATE(ins) do {\
	((struct tch_sdio_handle_prototype*) ins)->status &= ~0xffff;\
}while(0)


#define SDIO_ISVALID(ins)	((((struct tch_sdio_handle_prototype*) ins)->status & 0xffff) == (((uint32_t) ins ^ SDIO_CLASS_KEY) & 0xffff))


#define SET_BUSY(sdio) do {\
	set_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status |= SDIO_HANDLE_FLAG_BUSY;\
}while(0)

#define CLR_BUSY(sdio) do {\
	clear_system_busy();\
	((struct tch_sdio_handle_prototype*) sdio)->status &= ~SDIO_HANDLE_FLAG_BUSY;\
}while(0)

#define IS_BUSY(sdio) 					((struct tch_sdio_handle_prototype*) sdio)->status & SDIO_HANDLE_FLAG_BUSY

#define SET_DMA_MODE(sdio)				sdio->status |= SDIO_HANDLE_FLAG_DMA
#define CLR_DMA_MODE(sdio)				sdio->status &= ~SDIO_HANDLE_FLAG_DMA
#define IS_DMA_MODE(sdio)				(sdio->status & SDIO_HANDLE_FLAG_DMA)

#define IOERROR_FLAGS					(SDIO_STA_CCRCFAIL | \
										 SDIO_STA_DCRCFAIL | \
										 SDIO_STA_TXUNDERR | \
										 SDIO_STA_RXOVERR  | \
										 SDIO_STA_STBITERR)

#define IS_IOERROR(ev)					(ev & IOERROR_FLAGS)

typedef struct tch_sdio_device {
 	SdioDevType				type;
#define SDIO_DEV_FLAG_CAP_LARGE			((uint16_t) 1 << 1)		// if this is set, sd card is SDXC or SDEC class
#define SDIO_DEV_FLAG_S18A_SUPPORT		((uint16_t) 1 << 2)
#define SDIO_DEV_FLAG_UHSII_SUPPORT		((uint16_t) 1 << 3)
#define SDIO_DEV_FLAG_READY				((uint16_t) 1 << 4)
 	uint32_t				flags;
 	uint16_t 				caddr;
 	uint8_t					bus_width;
 	uint8_t					blksz;
 	uint32_t				csd[4];
}tch_sdioDev_t;



#define SDIO_REQ_TYPE_READ_BLK			((uint32_t) 1 << 0)
#define SDIO_REQ_TYPE_WRITE_BLK			((uint32_t) 1 << 1)
#define SDIO_REQ_MULTIBLK				((uint32_t) 1 << 2)
#define SDIO_REQ_NOK					((uint32_t) 1 << 3)

typedef struct tch_sdio_req {
	uint32_t				*buffer;
	uint32_t				 bsz;
	uint32_t				 bidx;
	uint32_t				 bcnt;
	uint32_t				 flag;
} tch_sdio_req_t;

struct tch_sdio_handle_prototype  {
	struct tch_sdio_handle 				pix;
	uint32_t 							status;
	uint16_t							vopt;
	uint8_t								dev_cnt;
	tch_sdioDev_t 						devs[MAX_SDIO_DEVCNT];
	tch_gpioHandle*						data_port;
	tch_gpioHandle*						cmd_port;
	tch_gpioHandle*						detect_port;
	tch_dmaHandle*						dma;
	tch_eventId							evId;
	const tch_core_api_t*				api;
	tch_sdio_req_t*						req;
	tch_mtxId							mtx;
	tch_condvId							condv;
};


typedef struct sdio_lock_blk_header {
	uint8_t		flag;
	uint8_t		pwdlen;
} sdio_lock_blk_header_t;

typedef struct sdio_lock_blk {
	sdio_lock_blk_header_t	header;
	char					pwd;
} sdio_lock_blk_t;


#define SDIO_RESP_SHORT		((sdio_resp_t) 1)
#define SDIO_RESP_LONG		((sdio_resp_t) 2)
typedef uint8_t		sdio_resp_t;



/*** private function declaration ***/

static int tch_sdio_init();
static void tch_sdio_exit();


/**********************************************************************************
 ********************  private function declaration *******************************
 **********************************************************************************/
/**
 * 	void					*buffer;
	uint32_t				 bsz;
	uint32_t				 bidx;
	uint32_t				 bcnt;
	uint8_t					 req_type;
 */
static void sdio_build_readreq(tch_sdio_req_t* req,void* buffer,uint32_t bsize,uint32_t bcnt);
static void sdio_build_writereq(tch_sdio_req_t* req,void* buffer,uint32_t bsize,uint32_t bcnt);
static uint32_t sdio_mmc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt);
static tchStatus sdio_set_bus_width(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint8_t bw);
static tchStatus sdio_send_cmd(struct tch_sdio_handle_prototype* ins, uint8_t cmdidx, uint32_t arg, BOOL waitresp, sdio_resp_t rtype, uint32_t* resp,uint32_t timeout);
static tchStatus sd_select_device(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device);
static tchStatus sd_deselect_device(struct tch_sdio_handle_prototype* ins);
static tchStatus sd_read_csd(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint32_t* csd);
static tchStatus sdio_read_block(struct tch_sdio_handle_prototype* ins, uint8_t cmdIdx, uint32_t address, void* buffer, uint32_t blk_cnt, uint32_t blk_size,  uint32_t timeout);
static tchStatus sdio_write_block(struct tch_sdio_handle_prototype* ins, uint8_t cmdIdx, uint32_t address,const void* buffer, uint32_t blk_cnt,uint32_t blk_size, uint32_t timeout);



/**********************************************************************************
 ********************  public function declaration for module API *****************
 **********************************************************************************/

__USER_API__ static void tch_sdio_initCfg(tch_sdioCfg_t* cfg,uint8_t buswidth, tch_PwrOpt lpopt);
__USER_API__ static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api,const tch_sdioCfg_t* config,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_release(tch_sdioHandle_t sdio);


/**********************************************************************************
 ********************  public function declaration for handle API *****************
 **********************************************************************************/


/**
 * reset all device in SDIO bus
 */
__USER_API__ static tchStatus tch_sdio_handle_device_reset(tch_sdioHandle_t sdio);
__USER_API__ static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt);
__USER_API__ tchStatus tch_sdio_handle_getDevInfo(tch_sdioHandle_t sdio, tch_sdioDevId device, tch_sdio_info_t* info);
__USER_API__ static tchStatus tch_sdio_handle_prepare(tch_sdioHandle_t sdio, tch_sdioDevId device,uint8_t option);
__USER_API__ static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout);
__USER_API__ static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_offset,uint32_t blk_cnt);
__USER_API__ static tchStatus tch_sdio_handle_eraseForce(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static tchStatus tch_sdio_handle_setPassword(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* opwd, const char* npwd, BOOL lock);
__USER_API__ static tchStatus tch_sdio_handle_lock(tch_sdioHandle_t sdio, tch_sdioDevId device,const char* pwd);
__USER_API__ static tchStatus tch_sdio_handle_unlock(tch_sdioHandle_t sdio, tch_sdioDevId device, const char* pwd);
__USER_API__ static SdioDevType tch_sdio_handle_getDeviceType(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static BOOL tch_sdio_handle_isProtectEnabled(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static uint64_t tch_sdio_handle_getMaxBitrate(tch_sdioHandle_t sdio, tch_sdioDevId device);
__USER_API__ static uint64_t tch_sdio_handle_getCapacity(tch_sdioHandle_t sdio, tch_sdioDevId device);


__USER_RODATA__ tch_hal_module_sdio_t SDIO_Ops = {
		.initCfg = tch_sdio_initCfg,
		.alloc = tch_sdio_alloc,
		.release = tch_sdio_release

};

static tch_mtxCb mtx;
static tch_condvCb condv;

static tch_hal_module_dma_t* dma;
static tch_hal_module_gpio_t* gpio;

// transfer rate unit
const uint32_t TransferUnit[4] = {
		100000,
		1000000,
		10000000,
		100000000
};

const float TransferTv[16] = {
	0,
	1.0f,
	1.2f,
	1.3f,
	1.5f,
	2.0f,
	2.5f,
	3.0f,
	3.5f,
	4.0f,
	4.5f,
	5.0f,
	5.5f,
	6.0f,
	7.0f,
	8.0f
};



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
	cfg->v_opt = SDIO_VO31_32 | SDIO_VO32_33;
}

static tch_sdioHandle_t tch_sdio_alloc(const tch_core_api_t* api, const tch_sdioCfg_t* config,uint32_t timeout)
{
	if(!config)
		return NULL;

	struct tch_sdio_handle_prototype* ins = NULL;
	uint32_t reg;
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

	// this guarantee all uninitialized member set to NULL
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
		dmacfg.Dir = DMA_Dir_PeriphToMem;
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
	else
	{
		CLR_DMA_MODE(ins);
		//TODO : implement Non DMA mode
	}

	// set ops for SDIO handle
	mset(ins->devs, 0, sizeof(struct tch_sdio_device) * MAX_SDIO_DEVCNT);

	ins->pix.deviceReset = tch_sdio_handle_device_reset;
	ins->pix.deviceId = tch_sdio_handle_device_id;
	ins->pix.getDevInfo = tch_sdio_handle_getDevInfo;
	ins->pix.prepare = tch_sdio_handle_prepare;
	ins->pix.getDeviceType = tch_sdio_handle_getDeviceType;
	ins->pix.isProtectEnabled = tch_sdio_handle_isProtectEnabled;
	ins->pix.getMaxBitrate = tch_sdio_handle_getMaxBitrate;
	ins->pix.getCapacity = tch_sdio_handle_getCapacity;
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
	ins->vopt = config->v_opt;
	ins->dev_cnt = 0;

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
		sdio_reg->MASK |= (SDIO_MASK_RXDAVLIE);
	}
	// enable error handling interrupt
	sdio_reg->MASK |= (SDIO_MASK_RXOVERRIE | SDIO_MASK_TXUNDERRIE |
						SDIO_MASK_CMDRENDIE | SDIO_MASK_CMDSENTIE |
						SDIO_MASK_CCRCFAILIE | SDIO_MASK_DBCKENDIE |
						SDIO_MASK_DATAENDIE | SDIO_MASK_DCRCFAILIE |
						SDIO_MASK_CTIMEOUTIE | SDIO_MASK_DTIMEOUTIE);

	reg = SDIO_CLKCR_PWRSAV;// | SDIO_CLKCR_HWFC_EN;			// power save mode enabled
	reg |= (SDIO_CLKCR_CLKDIV & 198);	// set clock divisor 48MHz / 200  because should be less than 400kHz
	reg |= SDIO_CLKCR_CLKEN;			// set sdio clock enable

	sdio_reg->CLKCR = reg;

	// set clock register for Identification phase
	ins->api = api;
	tch_enableInterrupt(sdio_hw->irq, PRIORITY_2, NULL);
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


static tchStatus tch_sdio_handle_device_reset(tch_sdioHandle_t sdio)
{
	if(!sdio)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	return sdio_send_cmd(ins, CMD_GO_IDLE_STATE, 0, FALSE, SDIO_RESP_SHORT, NULL, MAX_TIMEOUT_MILLS);			// send reset
}


static uint32_t tch_sdio_handle_device_id(tch_sdioHandle_t sdio,SdioDevType type,tch_sdioDevId* devIds,uint32_t max_Idcnt)
{
	if(!sdio || !devIds)
		return 0;
	if(!SDIO_ISVALID(sdio))
		return 0;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	if(ins->api->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
		return 0;
	while(IS_BUSY(ins))
	{
		if(ins->api->Condv->wait(ins->condv, ins->mtx, tchWaitForever) != tchOK)
			return 0;
	}
	SET_BUSY(ins);
	if(ins->api->Mtx->unlock(ins->mtx) != tchOK)
	{
		CLR_BUSY(ins);
		return 0;
	}
	uint32_t resp[4];



	switch(type){
	case MMC:
		resp[0] = sdio_mmc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	case SDC:
		resp[0] = sdio_sdc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	case SDIOC:
		resp[0] = sdio_sdioc_device_id((struct tch_sdio_handle_prototype*)sdio, devIds, max_Idcnt);
		break;
	default:
		resp[0] = 0;
	}

	ins->api->Mtx->lock(ins->mtx,tchWaitForever);
	CLR_BUSY(ins);
	ins->api->Condv->wakeAll(ins->condv);
	ins->api->Mtx->unlock(ins->mtx);
	return resp[0];
}

tchStatus tch_sdio_handle_getDevInfo(tch_sdioHandle_t sdio, tch_sdioDevId device, tch_sdio_info_t* info)
{
	if(!sdio || !device || !info)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;

	tch_sdioDev_t* dev = (tch_sdioDev_t*) device;
	if((info->ver = (dev->csd[0] & CSD_VERSION) >> 30) == 0)
	{
		info->tr_spd = dev->csd[0] & CSDv1_TRANS_SPEED;
		info->tr_spd = TransferUnit[info->tr_spd & 7] * TransferTv[info->tr_spd >> 3];

		info->rd_blen = (dev->csd[1] & CSDv1_RD_BLKLEN) >> 16;
		info->is_partial_rd_allowed = (dev->csd[1] & CSDv1_RD_PART_OK) >> 15;
		info->capacity = (dev->csd[1] & CSDv1_CSIZE_UPPER) << 2;
		info->capacity |= ((dev->csd[2] & CSDv1_CSIZE_LOWER) >> 30);
		info->capacity++;
		info->capacity *= (1 << (((dev->csd[2] & CSDv1_CSIZE_MUL) >> 15) + 2));
		info->capacity *= (1 << info->rd_blen);
		info->sect_size = ((dev->csd[2] & CSDv1_SECT_SIZE) >> 7);
		info->is_erase_blk_enable = ((dev->csd[2] & CSDv1_ERASE_BLK_EN) != 0);

		info->wr_blen = ((dev->csd[3] & CSDv1_WR_BLKLEN) >> 22);
		info->is_partial_wr_allowed = ((dev->csd[3] & CSDv1_WR_PART_OK) >> 21);
		info->tmp_wp = ((dev->csd[3] & CSDv1_TMP_WP) != 0);
		info->perm_wp = ((dev->csd[3] & CSDv1_PERM_WP) != 0);
		info->file_fmt = ((dev->csd[3] & CSDv1_FILE_FMT) >> 10);
	}
	else
	{
		info->tr_spd = dev->csd[0] & CSDv2_TRANS_SPEED;
		info->tr_spd = TransferUnit[info->tr_spd & 7] * TransferTv[info->tr_spd >> 3];

		info->rd_blen = (dev->csd[1] & CSDv2_RD_BLKLEN) >> 16;
		info->is_partial_rd_allowed = (dev->csd[1] & CSDv2_RD_PART_OK) >> 15;
		info->capacity = ((dev->csd[1] & CSDv2_CSIZE_UPPER) << 16);
		info->capacity |= ((dev->csd[2] & CSDv2_CSIZE_LOWER) >> 16);
		info->capacity++;
		info->capacity *= (512 * 1024);
		info->sect_size = ((dev->csd[2] & CSDv2_SECT_SIZE) >> 7);
		info->is_erase_blk_enable = ((dev->csd[2] & CSDv2_ERASE_BLK_EN) != 0);

		info->wr_blen = ((dev->csd[3] & CSDv2_WR_BLKLEN) >> 22);
		info->is_partial_wr_allowed = ((dev->csd[3] & CSDv2_WR_PART_OK) >> 21);
		info->tmp_wp = ((dev->csd[3] & CSDv2_TMP_WP) != 0);
		info->file_fmt = ((dev->csd[3] & CSDv2_FILE_FMT) >> 10);
	}

	return tchOK;
}


static tchStatus tch_sdio_handle_prepare(tch_sdioHandle_t sdio, tch_sdioDevId device,uint8_t option)
{
	if(!sdio || !device)
		return tchErrorParameter;
	if(!SDIO_ISVALID(sdio))
		return tchErrorParameter;

	tchStatus res;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;

	if((res = ins->api->Mtx->lock(ins->mtx, tchWaitForever)) != tchOK)
		return res;
	while(IS_BUSY(sdio))
	{
		if((res = ins->api->Condv->wait(ins->condv, ins->mtx, tchWaitForever) != tchOK))
		{
			return res;
		}
	}
	SET_BUSY(sdio);
	if((res = ins->api->Mtx->unlock(ins->mtx)) != tchOK)
		return res;

	uint32_t clkcr;
	uint32_t resp;
	SDIO_TypeDef* sdio_reg = SDIO_HWs[0]._sdio;
	tch_sdioDev_t* dev = (tch_sdioDev_t*) device;

	switch (option) {
	case SDIO_PREPARE_OPT_MAX_COMPAT:
		// TODO : sdio prepare for max compatibility
		// set data bus 1
		// set clock 24mhz
		clkcr = sdio_reg->CLKCR;
		clkcr &= ~(SDIO_CLKCR_BYPASS | SDIO_CLKCR_WIDBUS | SDIO_CLKCR_CLKDIV);
		sdio_set_bus_width(ins, device, 4);
		clkcr |= ((1 << 11) | SDIO_CLKCR_CLKEN);
		sdio_reg->CLKCR = clkcr;
		break;
	case SDIO_PREPARE_OPT_SPEED:
		// TODO : sdio
		//try to set data bus width to 4
		// if fail set data bus width to 1
		//set clock 48mhz
		clkcr = sdio_reg->CLKCR;
		clkcr &= ~(SDIO_CLKCR_BYPASS | SDIO_CLKCR_WIDBUS | SDIO_CLKCR_CLKDIV);
		sdio_set_bus_width(ins, device, 4);
		clkcr |= ((1 << 11) | SDIO_CLKCR_BYPASS | SDIO_CLKCR_CLKEN);
		sdio_reg->CLKCR = clkcr;
		break;
	}

	// negotiate data length for Standard class
	if(!(dev->flags & SDIO_DEV_FLAG_CAP_LARGE))
	{
		sd_select_device(ins,device);
		do {
			sdio_send_cmd(ins,CMD_SET_BLKLEN,(1 << dev->blksz),TRUE, SDIO_RESP_SHORT,&resp,MAX_TIMEOUT_MILLS);
			dev->blksz--;

			if(dev->blksz == 0)
			{
				//if fail to find proper block size
				return tchErrorIo;
			}
		}while(resp & SDC_STATUS_BLEN_ERR);
		dev->blksz++;
		sd_deselect_device(ins);
	}else{
		dev->blksz = 9;
	}


	if((res = ins->api->Mtx->lock(ins->mtx, tchWaitForever)) != tchOK)
		return res;
	CLR_BUSY(sdio);
	ins->api->Condv->wake(ins->condv);
	ins->api->Mtx->unlock(ins->mtx);

	return tchOK;
}


static SdioDevType tch_sdio_handle_getDeviceType(tch_sdioHandle_t sdio, tch_sdioDevId device)
{
	if(!sdio || !device)
		return UNKNOWN;
	if(!SDIO_ISVALID(sdio))
		return UNKNOWN;
	return ((tch_sdioDev_t*) device)->type;
}

static BOOL tch_sdio_handle_isProtectEnabled(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}

static uint64_t tch_sdio_handle_getMaxBitrate(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}
static uint64_t tch_sdio_handle_getCapacity(tch_sdioHandle_t sdio, tch_sdioDevId device)
{

}


static tchStatus tch_sdio_handle_writeBlock(tch_sdioHandle_t sdio,tch_sdioDevId device ,const uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout)
{
	if(!sdio || !SDIO_ISVALID(sdio))
		return tchErrorParameter;
	tchStatus res;
	uint32_t resp;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	const tch_core_api_t* api = ins->api;
	tch_sdioDev_t* dev = (tch_sdioDev_t*) device;
	uint8_t cmd;
	if((res = api->Mtx->lock(ins->mtx, timeout)) != tchOK)
	{
		return res;
	}
	// mutex successfully locked
	// wait until sdio idle
	while(IS_BUSY(sdio))
	{
		if((res = api->Condv->wait(ins->condv,ins->mtx, timeout)) != tchOK)
		{
			return res;
		}
	}
	SET_BUSY(sdio);
	if((res = api->Mtx->unlock(ins->mtx)) != tchOK)
	{
		return res;
	}

	res = sd_select_device(ins, dev);

	if((dev->type == SDC) && !(dev->flags & SDIO_DEV_FLAG_CAP_LARGE))
	{
		// if device is SD card and standard capacity
		// address should be bytes offset
		/**
		 * Physical Layer Simplified Version 4.10
		 * SDSC Card (CCS=0) uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 bytes unit).
		 */
		blk_offset <<= dev->blksz;
	}
	sdio_send_cmd(ins, CMD_SET_BLKLEN, 1 << dev->blksz, TRUE, SDIO_RESP_SHORT, &resp, timeout);

	if(blk_cnt > 1)
	{
		sdio_send_cmd(ins, CMD_APP_CMD , (dev->caddr << 16) ,TRUE , SDIO_RESP_SHORT,&resp,timeout);
		sdio_send_cmd(ins, CMD_SET_BLKCNT, blk_cnt, FALSE, SDIO_RESP_SHORT, &resp, timeout);
	}

	if(blk_cnt > 1)
	{
		cmd = CMD_WRITE_MBLK;
	}
	else
	{
		cmd = CMD_WRITE_SBLK;
	}
	res = sdio_write_block(ins, cmd, blk_offset, blk_bp, blk_cnt, dev->blksz,timeout);

	sd_deselect_device(ins);

	api->Mtx->lock(ins->mtx,tchWaitForever);
	CLR_BUSY(sdio);
	api->Condv->wakeAll(ins->condv);
	api->Mtx->unlock(ins->mtx);
	return res;
}

/**
 *
 */
static tchStatus tch_sdio_handle_readBlock(tch_sdioHandle_t sdio,tch_sdioDevId device,uint8_t* blk_bp,uint32_t blk_offset,uint32_t blk_cnt,uint32_t timeout)
{
	if(!sdio || !SDIO_ISVALID(sdio))
		return tchErrorParameter;
	tchStatus res = tchOK;
	uint8_t cmd;

	uint32_t resp = 0;
	struct tch_sdio_handle_prototype* ins = (struct tch_sdio_handle_prototype*) sdio;
	const tch_core_api_t* api = ins->api;
	tch_sdioDev_t *dev = (tch_sdioDev_t*) device;
	SDIO_TypeDef* sdio_reg = (SDIO_TypeDef*) SDIO_HWs[0]._sdio;

	if((res = api->Mtx->lock(ins->mtx, timeout)) != tchOK)
		return res;
	while(IS_BUSY(sdio))
	{
		if((res = api->Condv->wait(ins->condv, ins->mtx, timeout)) != tchOK)
			return res;
	}
	SET_BUSY(sdio);
	if((res = api->Mtx->unlock(ins->mtx)) != tchOK)
	{
		return res;
	}

	while((res = sd_select_device(ins, dev)) != tchOK)
	{
		if(res == tchEventTimeout)
		{
			return res;
		}
		// wait until device ready
		api->Thread->yield(0);
	}

	if((dev->type == SDC) && !(dev->flags & SDIO_DEV_FLAG_CAP_LARGE))
	{
		// if device is SD card and standard capacity
		// address should be bytes offset
		/**
		 * Physical Layer Simplified Version 4.10
		 * SDSC Card (CCS=0) uses byte unit address and SDHC and SDXC Cards (CCS=1) use block unit address (512 bytes unit).
		 */
		blk_offset <<= dev->blksz;
	}

	sdio_send_cmd(ins, CMD_SET_BLKLEN, 1 << dev->blksz, TRUE, SDIO_RESP_SHORT, &resp, timeout);

	if(blk_cnt > 1)
	{
		sdio_send_cmd(ins, CMD_APP_CMD, (dev->caddr << 16), TRUE, SDIO_RESP_SHORT,&resp,timeout);
		sdio_send_cmd(ins, CMD_SET_BLKCNT, blk_cnt, TRUE, SDIO_RESP_SHORT, &resp, timeout);
	}


	if(blk_cnt > 1)
	{
		cmd = CMD_READ_MBLK;
	}
	else
	{
		cmd = CMD_READ_SBLK;
	}
	res = sdio_read_block(ins, cmd, blk_offset, blk_bp, blk_cnt, dev->blksz,timeout);

	sd_deselect_device(ins);

	api->Mtx->lock(ins->mtx,tchWaitForever);
	CLR_BUSY(sdio);
	api->Condv->wake(ins->condv);
	api->Mtx->unlock(ins->mtx);
	return res;
}

static tchStatus tch_sdio_handle_erase(tch_sdioHandle_t sdio,tch_sdioDevId device, uint32_t blk_offset,uint32_t blk_cnt)
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

static void sdio_build_readreq(tch_sdio_req_t* req,void* buffer,uint32_t bsize,uint32_t bcnt)
{
	req->bcnt = bcnt;
	req->bsz = bsize;
	req->buffer = buffer;
	req->flag = SDIO_REQ_TYPE_READ_BLK;
	if(bcnt > 1)
		req->flag |= SDIO_REQ_MULTIBLK;
	req->bidx = 0;
}

static void sdio_build_writereq(tch_sdio_req_t* req,void* buffer,uint32_t bsize,uint32_t bcnt)
{
	req->bcnt = bcnt;
	req->bsz = bsize;
	req->buffer = buffer;
	req->flag = SDIO_REQ_TYPE_WRITE_BLK;
	if(bcnt > 1)
		req->flag |= SDIO_REQ_MULTIBLK;
	req->bidx = 0;
}

static uint32_t sdio_sdc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{
	const tch_core_api_t* api = ins->api;

	//ensure max identification count less than MAX_SDIO_DEVCNT
	if(max_idcnt > MAX_SDIO_DEVCNT)
		max_idcnt = MAX_SDIO_DEVCNT;

	tchStatus res = tchOK;
	uint32_t pdevcnt = ins->dev_cnt;
	uint32_t arg = 0;
	uint32_t resp[4];


	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;

	// SEND_IF_COND to all devices and only the cards that support the condition become available
	sdio_send_cmd(ins, CMD_SEND_IF_COND, 0x1FF, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
	if(resp[0] != 0x1FF)
		return tchErrorIo;				/// there is no card to

	while(res != tchEventTimeout && (ins->dev_cnt < max_idcnt))
	{
		/* continue card init process until timeout happens or the number of found device
		 * exceeds max_idcnt
		*/

		// send card initialization command with OCR setting
		/**
		 * send SEND_OP_COND to all cards serially response to it.
		 * SEND_OP_COND is actually sent to device 2 times.
		 * 1. host request operating condition to device
		 * 2. a device replies its operating condition
		 * 3. host confirms the operating condition by sending another SEND_OP_COND with the same argument
		 * 4. wait until device is ready
		 */
		res = tchOK;
		sdio_send_cmd(ins, CMD_APP_CMD, 0, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);
		arg = SDIO_ACMD41_ARG_HCS;
		arg |= (ins->vopt << SDIO_VO_TO_VW_SHIFT);
		res = sdio_send_cmd(ins, ACMD_SEND_OP_COND, arg , TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);

		if((resp[0] & SDIO_R3_READY))
		{
			mset(&ins->devs[ins->dev_cnt], 0 ,sizeof(struct tch_sdio_device));			// clear device info
			ins->devs[ins->dev_cnt].type = SDC;

			if(resp[0] & SDIO_R3_UHSII)
				ins->devs[ins->dev_cnt].flags |= SDIO_DEV_FLAG_UHSII_SUPPORT;
			if(resp[0] & SDIO_R3_CCS)
				ins->devs[ins->dev_cnt].flags |= SDIO_DEV_FLAG_CAP_LARGE;

			resp[0] = 0;
			sdio_send_cmd(ins, CMD_ALL_SEND_CID, 0, TRUE, SDIO_RESP_LONG, resp, MAX_TIMEOUT_MILLS);		// request CID

			resp[0] = 0;
			sdio_send_cmd(ins, CMD_SEND_RCA, 0, TRUE, SDIO_RESP_SHORT, resp, MAX_TIMEOUT_MILLS);		// request RCA Publish
			ins->devs[ins->dev_cnt].caddr = (resp[0] >> 16);
			devIds[ins->dev_cnt] = &ins->devs[ins->dev_cnt];

			sd_read_csd(ins, &ins->devs[ins->dev_cnt], ins->devs[ins->dev_cnt].csd);
			ins->devs[ins->dev_cnt].blksz = (ins->devs[ins->dev_cnt].csd[1] & CSDv1_RD_BLKLEN) >> 16;
			ins->devs[ins->dev_cnt].flags |= SDIO_DEV_FLAG_READY;
			ins->dev_cnt++;
		}
	}
	return ins->dev_cnt - pdevcnt;
}

static uint32_t sdio_sdioc_device_id(struct tch_sdio_handle_prototype* ins, tch_sdioDevId* devIds, uint32_t max_idcnt)
{

}

static tchStatus sdio_set_bus_width(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint8_t bw)
{
	if(!ins || !SDIO_ISVALID(ins) || !device)
		return tchErrorParameter;

	tch_sdioDev_t* dev = (tch_sdioDev_t*) device;
	uint32_t resp;
	tchStatus result = tchOK;

	if(!(dev->flags & SDIO_DEV_FLAG_READY))
		return tchErrorIo;
	/**
	 *  to configure bus width card should be in trasn state and unlocked
	 */
	if((result = sd_select_device(ins,dev)) != tchOK)
		return result;
	if((result = sdio_send_cmd(ins,CMD_APP_CMD, (dev->caddr) << 16,TRUE,SDIO_RESP_SHORT,&resp,MAX_TIMEOUT_MILLS)) != tchOK)
		return result;
	if((result = sdio_send_cmd(ins,ACMD_SET_BUS_WIDTH, (bw >> 1),TRUE,SDIO_RESP_SHORT,&resp,MAX_TIMEOUT_MILLS)) != tchOK)
		return result;
	if((result = sd_deselect_device(ins)) != tchOK)
		return result;
	dev->bus_width = bw;
	return result;
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
		res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDREND, timeout);
		if(resp)
		{
			switch(rtype){
			case SDIO_RESP_SHORT:
				resp[0] = sdio_reg->RESP1;
				break;
			case SDIO_RESP_LONG:
				mcpy(resp, &sdio_reg->RESP1, 4 * sizeof(uint32_t));
				break;
			}
		}
	}
	else
	{
		cmd |= SDIO_CMD_ENCMDCOMPL;
		sdio_reg->CMD = cmd;
		res = ins->api->Event->wait(ins->evId, SDIO_STA_CMDSENT, timeout);
	}
	ev = ins->api->Event->clear(ins->evId, SDIO_STA_CMDSENT | SDIO_STA_CMDREND | IOERROR_FLAGS);
	if(IS_IOERROR(ev))
	{
		res = tchErrorIo;
	}
	return res;
}

static tchStatus sd_select_device(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	struct tch_sdio_device* dinfo = (struct tch_sdio_device*) device;
	uint32_t cstatus = 0;
	tchStatus res = tchOK;

	if((res = sdio_send_cmd(ins,CMD_SELECT, (dinfo->caddr << 16), TRUE, SDIO_RESP_SHORT, &cstatus, MAX_TIMEOUT_MILLS)) != tchOK)
	{
		return res;
	}
	if(cstatus & SDC_STATUS_DATA_RDY)
		return tchOK;
	return tchErrorIo;
}

static tchStatus sd_deselect_device(struct tch_sdio_handle_prototype* ins)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	return sdio_send_cmd(ins,CMD_SELECT,0,FALSE,SDIO_RESP_SHORT,NULL,MAX_TIMEOUT_MILLS);
}
static tchStatus sd_read_csd(struct tch_sdio_handle_prototype* ins, tch_sdioDevId device,uint32_t* csd)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;
	struct tch_sdio_device* dinfo = (struct tch_sdio_device*) device;
	return sdio_send_cmd(ins,CMD_SEND_CSD,(dinfo->caddr << 16), TRUE,SDIO_RESP_LONG, csd,MAX_TIMEOUT_MILLS);
}



/**
 * read block helper function
 * @param[in] ins	sdio handle
 * @param[in] cmdIdx cmd index
 * @param[in] buffer read buffer
 * @param[in] blk_cnt the number of block to be read
 * @param[in] blk_size the exponent of block size (ex. 512 byte = 9)
 * @param[in] timeout timeout in millisec
 */
static tchStatus sdio_read_block(struct tch_sdio_handle_prototype* ins, uint8_t cmdIdx, uint32_t address, void* buffer, uint32_t blk_cnt, uint32_t blk_size,  uint32_t timeout)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;

	uint32_t resp = 0;
	int32_t ev = 0;
	tchStatus res;
	SDIO_TypeDef* sdio_reg = (SDIO_TypeDef*) SDIO_HWs[0]._sdio;
	tch_sdio_req_t request;
	sdio_build_readreq(&request,buffer,blk_cnt << blk_size,blk_cnt);
	ins->req = &request;

	sdio_reg->DCTRL = 0;					// data state machine disable
	sdio_reg->CLKCR &= ~SDIO_CLKCR_PWRSAV;	// disable power save mode

	if(ins->status & SDIO_HANDLE_FLAG_DMA)
	{
		// DMA Used to receive data
		tch_DmaReqDef dma_req;
		dma->initReq(&dma_req, buffer, (char*) &sdio_reg->FIFO, 0 ,DMA_Dir_PeriphToMem);
		if((res = dma->beginXferAsync(ins->dma,&dma_req)) != tchOK)
		{
			goto READ_ERR_RET;
		}

		sdio_reg->DLEN = blk_cnt << blk_size;
		sdio_reg->DTIMER = timeout << 4;

		sdio_reg->DCTRL = (SDIO_DCTRL_DMAEN |  SDIO_DCTRL_DTDIR |(blk_size << 4));
		sdio_reg->DCTRL |= SDIO_DCTRL_DTEN;

		if((res = sdio_send_cmd(ins, cmdIdx, address,TRUE,SDIO_RESP_SHORT,&resp,timeout)) != tchOK)
		{
			goto READ_ERR_RET;
		}
		dma->waitComplete(ins->dma, timeout);
	}
	else
	{
		/**
		 *  Note : multiple blocks of reading without using DMA cuases
		 *         overrun error, however, single block reading has been working
		 *         without any problem so far. so if DMA can't be allocated to SDIO
		 *         single block of reading can be used.
		 */
		// enable rx fifo half full interrupt
		sdio_reg->MASK |= (SDIO_MASK_RXDAVLIE);
		sdio_reg->DLEN = blk_cnt << blk_size;
		sdio_reg->DTIMER = timeout << 16;
		request.bcnt = 1;

		Thread->yield(0);

		sdio_reg->DCTRL = (SDIO_DCTRL_DTDIR | (blk_size << 4));
		sdio_reg->DCTRL |= SDIO_DCTRL_DTEN;

		if((res = sdio_send_cmd(ins, cmdIdx, address, TRUE, SDIO_RESP_SHORT,&resp,timeout)) != tchOK)
		{
			goto READ_ERR_RET;
		}
	}

	res = ins->api->Event->wait(ins->evId, SDIO_STA_DBCKEND | SDIO_STA_DATAEND , timeout);
	ev = ins->api->Event->clear(ins->evId, SDIO_STA_DBCKEND | SDIO_STA_DATAEND  | IOERROR_FLAGS);

	// disable rx fifo half full interrupt
READ_ERR_RET:

	if(IS_IOERROR(ev))
	{
		res = tchErrorIo;
	}
	if((blk_cnt > 1) || (res != tchOK))
	{
		// SD Card will not respond, if it is already in transfer state
		// so doesn't wait response
		sdio_send_cmd(ins,CMD_STOP_TRANS,0, FALSE, SDIO_RESP_SHORT,&resp, timeout);
	}

	sdio_reg->DCTRL = 0;
	ins->req = NULL;
	sdio_reg->CLKCR |= SDIO_CLKCR_PWRSAV;
	sdio_reg->MASK &= ~(SDIO_MASK_RXDAVLIE);
	return res;
}


static tchStatus sdio_write_block(struct tch_sdio_handle_prototype* ins, uint8_t cmdIdx, uint32_t address,const void* buffer, uint32_t blk_cnt,uint32_t blk_size, uint32_t timeout)
{
	if(!ins || !SDIO_ISVALID(ins))
		return tchErrorParameter;

	tchStatus result;
	uint32_t resp;
	int32_t ev = 0;
	const tch_core_api_t* api = ins->api;
	SDIO_TypeDef* sdio_reg = SDIO_HWs[0]._sdio;
	tch_sdio_req_t request;

	sdio_build_writereq(&request, (void*) buffer, blk_cnt << blk_size, blk_cnt);
	ins->req = &request;

	api->Event->clear(ins->evId, 0xffffffff);

	sdio_reg->DCTRL = 0;
	sdio_reg->CLKCR &= ~SDIO_CLKCR_PWRSAV;


	if(ins->status & SDIO_HANDLE_FLAG_DMA)
	{
		tch_DmaReqDef req;


		dma->initReq(&req,(uwaddr_t) buffer, (uwaddr_t) &sdio_reg->FIFO, blk_cnt << blk_size, DMA_Dir_MemToPeriph);
		dma->beginXferAsync(ins->dma,&req);

		if((result = sdio_send_cmd(ins,cmdIdx,address, TRUE, SDIO_RESP_SHORT, &resp, timeout)) != tchOK)
		{
			goto WRITE_ERR_RET;
		}

		sdio_reg->DLEN = blk_cnt << blk_size;
		sdio_reg->DTIMER = timeout << 16;

		sdio_reg->DCTRL = (SDIO_DCTRL_DMAEN | (blk_size << 4));
		sdio_reg->DCTRL |= SDIO_DCTRL_DTEN;




		dma->waitComplete(ins->dma,timeout);

	}
	else
	{

	}

	result = api->Event->wait(ins->evId, SDIO_STA_DBCKEND | SDIO_STA_DATAEND, timeout);
	ev = api->Event->clear(ins->evId, SDIO_STA_DBCKEND | SDIO_STA_DATAEND | IOERROR_FLAGS);

WRITE_ERR_RET:

	if(IS_IOERROR(ev))
	{
		result = tchErrorIo;
	}

	if((blk_cnt > 1) || (result != tchOK))
	{
			// SD Card will not respond, if it is already in transfer state
			// so doesn't wait response
			sdio_send_cmd(ins,CMD_STOP_TRANS,0, FALSE, SDIO_RESP_SHORT,&resp, timeout);
	}

	sdio_reg->DCTRL = 0;
	ins->req = NULL;
	sdio_reg->CLKCR |= SDIO_CLKCR_PWRSAV;
	return result;
}


// interrupt handler
void SDIO_IRQHandler()
{
	const tch_sdio_descriptor* sdio_hw = &SDIO_HWs[0];
	struct tch_sdio_handle_prototype* sdio_ins = sdio_hw->_handle;
	const tch_core_api_t* api = sdio_ins->api;
	SDIO_TypeDef* sdio_reg = sdio_hw->_sdio;
	uint32_t cnt;
	uint32_t dummy;
	if(sdio_reg->STA & SDIO_STA_RXFIFOF)
	{
		sdio_reg->DCTRL |= SDIO_DCTRL_RWSTART;
		sdio_reg->MASK |= SDIO_MASK_RXFIFOEIE;
	}
	if(sdio_reg->STA & SDIO_STA_RXDAVL)
	{
		while (sdio_reg->STA & SDIO_STA_RXDAVL)
		{
			sdio_ins->req->buffer[sdio_ins->req->bidx++] = sdio_reg->FIFO;
			if ((sdio_ins->req->bsz >> 2) <= sdio_ins->req->bidx)
			{
				dummy = sdio_reg->FIFO;
			}
		}
		return;
	}
	if(sdio_reg->STA & SDIO_STA_RXFIFOE)
	{
		sdio_reg->DCTRL |= SDIO_DCTRL_RWSTOP;
		sdio_reg->MASK &= ~SDIO_MASK_RXFIFOEIE;
	}
	if(sdio_reg->STA & SDIO_STA_RXOVERR)
	{
		// rx overrun error
		sdio_reg->ICR |= SDIO_ICR_RXOVERRC;
		api->Event->set(sdio_ins->evId, SDIO_STA_RXOVERR);
		return;
	}
	if(sdio_reg->STA & SDIO_STA_TXUNDERR)
	{
		// tx underrun error
		sdio_reg->ICR |= SDIO_ICR_TXUNDERRC;
		api->Event->set(sdio_ins->evId, SDIO_STA_TXUNDERR);
		return;
	}
	if(sdio_reg->STA & SDIO_STA_CMDSENT)
	{
		sdio_reg->ICR |= SDIO_ICR_CMDSENTC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CMDSENT);
		api->Dbg->print(api->Dbg->Normal, 0, "SDIO Command (%d) Sent\n\r", (sdio_reg->CMD & SDIO_CMD_CMDINDEX));
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
		api->Event->set(sdio_ins->evId, SDIO_STA_CCRCFAIL | SDIO_STA_CMDREND);
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Command (%d) Responded\n\r wiht CRC Error", (sdio_reg->RESPCMD & SDIO_RESPCMD_RESPCMD));
		return;
	}
	if(sdio_reg->STA & SDIO_STA_DBCKEND)
	{
		sdio_reg->ICR |= SDIO_ICR_DBCKENDC;
		if(!--sdio_ins->req->bcnt)
		{
			//when block count reaches '0', wakes up thread
			api->Event->set(sdio_ins->evId, SDIO_STA_DBCKEND);
		}
		api->Dbg->print(api->Dbg->Normal,0, "SDIO Block op. complete\n\r");
		return;
	}
	if(sdio_reg->STA & SDIO_STA_DATAEND)
	{
		sdio_reg->ICR |= SDIO_ICR_DATAENDC;
		api->Event->set(sdio_ins->evId, SDIO_STA_DATAEND);
		return;
	}

	if(sdio_reg->STA & SDIO_STA_DCRCFAIL)
	{
		sdio_reg->ICR |= SDIO_ICR_DCRCFAILC;
		api->Event->set(sdio_ins->evId, SDIO_STA_DCRCFAIL);
		return;
	}
	if(sdio_reg->STA & SDIO_STA_DTIMEOUT)
	{
		sdio_reg->ICR |= SDIO_ICR_DTIMEOUTC;
		api->Event->set(sdio_ins->evId, SDIO_STA_DTIMEOUT);
		return;
	}

	if(sdio_reg->STA & SDIO_STA_CTIMEOUT)
	{
		sdio_reg->ICR |= SDIO_ICR_CTIMEOUTC;
		api->Event->set(sdio_ins->evId, SDIO_STA_CTIMEOUT);
		return;
	}
}

