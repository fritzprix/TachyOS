/*
 * tch_dma.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 7.
 *      Author: innocentevil
 */


#include "tch_hal.h"
#include "tch_kernel.h"
#include "tch_dma.h"
#include "tch_halobjs.h"

#define DMA_MBurst_Pos            (uint8_t) 23
#define DMA_PBurst_Pos            (uint8_t) 21
#define DMA_Burst_Single          (uint8_t) 0
#define DMA_Burst_Inc4            (uint8_t) 1
#define DMA_Burst_Inc8            (uint8_t) 2
#define DMA_Burst_Inc16           (uint8_t) 3
#define DMA_Burst_Msk             (DMA_Burst_Single |\
		                           DMA_Burst_Inc4|\
		                           DMA_Burst_Inc8|\
		                           DMA_Burst_Inc16)

#define DMA_Minc_Pos              (uint8_t) 10
#define DMA_Minc_Enable           (uint8_t) 1
#define DMA_Minc_Disable          (uint8_t) 0

#define DMA_Pinc_Pos              (uint8_t) 9
#define DMA_Pinc_Enable           (uint8_t) 1
#define DMA_Pinc_Disable          (uint8_t) 0


#define DMA_Dir_Pos               (uint8_t) 6
#define DMA_Dir_PeriphToMem       (uint8_t) 0
#define DMA_Dir_MemToPeriph       (uint8_t) 1
#define DMA_Dir_MemToMem          (uint8_t) 2
#define DMA_Dir_Msk               (DMA_Dir_PeriphToMem |\
		                           DMA_Dir_MemToPeriph |\
		                           DMA_Dir_MemToMem)

#define DMA_MDataAlign_Pos        (uint8_t) 13
#define DMA_PDataAlign_Pos        (uint8_t) 11
#define DMA_DataAlign_Byte        (uint8_t) 0
#define DMA_DataAlign_Hword       (uint8_t) 1
#define DMA_DataAlign_Word        (uint8_t) 2
#define DMA_DataAlign_Msk         (DMA_DataAlign_Byte  |\
		                           DMA_DataAlign_Hword |\
		                           DMA_DataAlign_Word)

#define DMA_Prior_Pos            (uint8_t) 16
#define DMA_Prior_Low            (uint8_t) 0
#define DMA_Prior_Mid            (uint8_t) 1
#define DMA_Prior_High           (uint8_t) 2
#define DMA_Prior_VHigh          (uint8_t) 4


#define DMA_FlowControl_Pos      (uint8_t) 5
#define DMA_FlowControl_DMA      (uint8_t) 0
#define DMA_FlowControl_Periph   (uint8_t) 1



#define DMA_BufferMode_Normal     (uint8_t) 0
#define DMA_BufferMode_Dbl        (uint8_t) 1
#define DMA_BufferMode_Cir        (uint8_t) 2

#define DMA_TargetAddress_Mem0    (uint8_t) 1
#define DMA_TargetAddress_Periph  (uint8_t) 2
#define DMA_TargetAddress_Mem1    (uint8_t) 3


#define DMA_FLAG_EvPos                         (uint8_t)  3
#define DMA_FLAG_EvFE                          (uint16_t) (1 << 0)
#define DMA_FLAG_EvHT                          (uint16_t) (1 << 4)
#define DMA_FLAG_EvTC                          (uint16_t) (1 << 5)
#define DMA_FLAG_EvTE                          (uint16_t) (1 << 3)
#define DMA_FLAG_EvDME                         (uint16_t) (1 << 2)
#define DMA_FLAG_EvMsk                         (DMA_FLAG_EvFE |\
		                                        DMA_FLAG_EvHT |\
		                                        DMA_FLAG_EvTC |\
		                                        DMA_FLAG_EvTE |\
		                                        DMA_FLAG_EvDME)

#define INIT_DMA_BUFFER_TYPE                   {\
                                                 DMA_BufferMode_Normal,\
                                                 DMA_BufferMode_Dbl,\
                                                 DMA_BufferMode_Cir\
                                               }

#define INIT_DMA_DIR_TYPE                      {\
	                                             DMA_Dir_PeriphToMem,\
	                                             DMA_Dir_MemToMem,\
	                                             DMA_Dir_MemToPeriph\
                                               }

#define INIT_DMA_PRIORITY_TYPE                 {\
	                                             DMA_Prior_VHigh,\
	                                             DMA_Prior_High,\
	                                             DMA_Prior_Mid,\
	                                             DMA_Prior_Low\
                                                }

#define INIT_DMA_BURSTSIZE_TYPE                 {\
	                                              DMA_Burst_Single,\
	                                              0,\
	                                              DMA_Burst_Inc4,\
	                                              DMA_Burst_Inc8,\
	                                              DMA_Burst_Inc16\
                                                }


#define INIT_DMA_FLOWCTRL_TYPE                  {\
	                                              DMA_FlowControl_Periph,\
	                                              DMA_FlowControl_DMA\
                                                }

#define INIT_DMA_ALIGN_TYPE                     {\
	                                              DMA_DataAlign_Byte,\
	                                              DMA_DataAlign_Hword,\
	                                              DMA_DataAlign_Word\
                                                }





typedef struct tch_dma_manager_t {
	tch_dma_ix                 _pix;
	tch_mtx                    mtx;
	uint8_t                    occp_state[2];
	uint8_t                    lpoccp_state[2];
}tch_dma_manager;

typedef struct tch_dma_handle_prototype_t{
	tch_dma_handle             _pix;
	dma_t                       dma;
	uint8_t                     ch;
	tch_mtx                     mtx_head;
}tch_dma_handle_prototype;

static void tch_dma_initCfg(tch_dma_cfg* cfg);
static tch_dma_handle* tch_dma_openStream(dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg);


static BOOL tch_dma_beginXfer(tch_dma_handle* self,uint32_t size);
static void tch_dma_setAddress(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
static void tch_dma_registerEventListener(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
static void tch_dma_unregisterEventListener(tch_dma_handle* self);
static void tch_dma_setIncrementMode(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
static void tch_dma_close(tch_dma_handle* self);



__attribute__((section(".data"))) static tch_dma_manager DMA_Manager = {
		{
				INIT_DMA_BUFFER_TYPE,
				INIT_DMA_DIR_TYPE,
				INIT_DMA_PRIORITY_TYPE,
				INIT_DMA_BURSTSIZE_TYPE,
				INIT_DMA_FLOWCTRL_TYPE,
				INIT_DMA_ALIGN_TYPE,
				tch_dma_initCfg,
				tch_dma_openStream
		},
		INIT_MTX,
		{0},
		{0}
};

const tch_dma_ix* Dma = (tch_dma_ix*) &DMA_Manager;


static void tch_dma_initCfg(tch_dma_cfg* cfg){
	cfg->BufferType = Dma->BufferType.Normal;
	cfg->Ch = 0;
	cfg->Dir = Dma->Dir.MemToPeriph;
	cfg->FlowCtrl = Dma->FlowCtrl.DMA;
	cfg->Priority = Dma->Priority.Normal;
	cfg->mAlign = Dma->Align.Byte;
	cfg->pAlign = Dma->Align.Byte;
	cfg->mInc = FALSE;
	cfg->pInc = FALSE;
	cfg->mBurstSize = Dma->BurstSize.Burst1;
	cfg->pBurstSize = Dma->BurstSize.Burst1;
}


static tch_dma_handle* tch_dma_openStream(dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg){
	/// check H/W Occupation
	Mtx->lock(&DMA_Manager.mtx,timeout);
	if(dma < 8){
		if(DMA_Manager.occp_state[0] & (1 << dma)){
			Mtx->unlock(&DMA_Manager.mtx);
			return NULL;
		}
		DMA_Manager.occp_state[0] |= (1 << dma);
	}else{
		if(DMA_Manager.occp_state[1] & (1 << (dma - 8))){
			Mtx->unlock(&DMA_Manager.mtx);
			return NULL;
		}
		DMA_Manager.occp_state[1] |= (1 << (dma - 8));
	}
	Mtx->unlock(&DMA_Manager.mtx);

	// Acquire DMA H/w
	tch_dma_descriptor* dma_desc = &DMA_HWs[dma];
	*dma_desc->_clkenr |= dma_desc->clkmsk;
	if(pcfg == ActOnSleep)
		*dma_desc->_lpclkenr |= dma_desc->lpcklmsk;

	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)dma_desc->_hw;

}

static BOOL tch_dma_beginXfer(tch_dma_handle* self,uint32_t size){

}

static void tch_dma_setAddress(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr){

}

static void tch_dma_registerEventListener(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType){

}

static void tch_dma_unregisterEventListener(tch_dma_handle* self){

}

static void tch_dma_setIncrementMode(tch_dma_handle* self,uint8_t targetAddress,BOOL enable){

}

static void tch_dma_close(tch_dma_handle* self){

}

