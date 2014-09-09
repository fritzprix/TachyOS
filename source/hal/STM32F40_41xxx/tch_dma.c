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
#include "tch_halInit.h"


#define DMA_Str0                  (dma_t) 0    ///< DMA Stream #0
#define DMA_Str1                  (dma_t) 1    ///< DMA Stream #1
#define DMA_Str2                  (dma_t) 2    ///< DMA Stream #2
#define DMA_Str3                  (dma_t) 3    ///< DMA Stream #3
#define DMA_Str4                  (dma_t) 4    ///< DMA Stream #4
#define DMA_Str5                  (dma_t) 5    ///< DMA Stream #5
#define DMA_Str6                  (dma_t) 6    ///< DMA Stream #6
#define DMA_Str7                  (dma_t) 7    ///< DMA Stream #7
#define DMA_Str8                  (dma_t) 8    ///< DMA Stream #8
#define DMA_Str9                  (dma_t) 9    ///< DMA Stream #9
#define DMA_Str10                 (dma_t) 10   ///< DMA Stream #10
#define DMA_Str11                 (dma_t) 11   ///< DMA Stream #11
#define DMA_Str12                 (dma_t) 12   ///< DMA Stream #12
#define DMA_Str13                 (dma_t) 13   ///< DMA Stream #13
#define DMA_Str14                 (dma_t) 14   ///< DMA Stream #14
#define DMA_Str15                 (dma_t) 15   ///< DMA Stream #15
#define DMA_NOT_USED              (dma_t) -1   ///< DMA Stream is not used

#define DMA_Ch0                   (uint8_t) 0  ///< DMA Channel #0
#define DMA_Ch1                   (uint8_t) 1  ///< DMA Channel #1
#define DMA_Ch2                   (uint8_t) 2  ///< DMA Channel #2
#define DMA_Ch3                   (uint8_t) 3  ///< DMA Channel #3
#define DMA_Ch4                   (uint8_t) 4  ///< DMA Channel #4
#define DMA_Ch5                   (uint8_t) 5  ///< DMA Channel #5
#define DMA_Ch6                   (uint8_t) 6  ///< DMA Channel #6
#define DMA_Ch7                   (uint8_t) 7  ///< DMA Channel #7


#define DMA_Ch_Pos                (uint8_t) 25 ///< DMA Channel bit position
#define DMA_Ch_Msk                (7)


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

#define DMA_FLAG_BUSY                          ((uint16_t) (1 << 10))

#define INIT_DMA_STR_TYPE                      {\
	                                            DMA_Str0,\
	                                            DMA_Str1,\
	                                            DMA_Str2,\
	                                            DMA_Str3,\
	                                            DMA_Str4,\
	                                            DMA_Str5,\
	                                            DMA_Str6,\
	                                            DMA_Str7,\
	                                            DMA_Str8,\
	                                            DMA_Str9,\
	                                            DMA_Str10,\
	                                            DMA_Str11,\
	                                            DMA_Str12,\
	                                            DMA_Str13,\
	                                            DMA_Str14,\
	                                            DMA_Str15\
                                                }

#define INIT_DMA_CH_TYPE                       {\
	                                             DMA_Ch0,\
	                                             DMA_Ch1,\
	                                             DMA_Ch2,\
	                                             DMA_Ch3,\
	                                             DMA_Ch4,\
	                                             DMA_Ch5,\
	                                             DMA_Ch6,\
	                                             DMA_Ch7\
                                                }

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

#define INIT_DMA_TARGET_ADDRESS                 {\
	                                              DMA_TargetAddress_Mem0,\
	                                              DMA_TargetAddress_Mem1,\
	                                              DMA_TargetAddress_Periph,\
	                                              0\
                                                }





typedef struct tch_dma_manager_t {
	tch_dma_ix                 _pix;
	tch_mtxId                   mtxId;
	tch_condvId                 condvId;
	uint16_t                    occp_state;
	uint16_t                    lpoccp_state;
}tch_dma_manager;


#define TCH_DMA_CLASS_KEY             ((uint16_t) 0x3D01)
#define TCH_DMA_BUSY                  ((uint16_t) 1)

typedef struct tch_dma_handle_prototype_t{
	tch_dma_handle             _pix;
	tch*                        api;
	dma_t                       dma;
	uint8_t                     ch;
	tch_mtxId                   mtxId;
	tch_condvId                 condv;
	tch_lnode_t                 wq;
	tch_dma_eventListener       listener;
	uint32_t                    status;
	tch_async_id                dma_async;
}tch_dma_handle_prototype;

static void tch_dma_initCfg(tch_dma_cfg* cfg);
static tch_dma_handle* tch_dma_openStream(tch* sys,dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg);


static BOOL tch_dma_beginXfer(tch_dma_handle* self,uint32_t size,uint32_t timeout);
static void tch_dma_setAddress(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
static void tch_dma_registerEventListener(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
static void tch_dma_unregisterEventListener(tch_dma_handle* self);
static void tch_dma_setIncrementMode(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
static void tch_dma_close(tch_dma_handle* self);
static tch_dma_handle* tch_dma_initHandle(dma_t dma,uint8_t ch);
static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc);

static DECL_ASYNC_TASK(tch_dma_trigger);


__attribute__((section(".data"))) static tch_dma_manager DMA_Manager = {
		{
				INIT_DMA_STR_TYPE,
				INIT_DMA_CH_TYPE,
				INIT_DMA_BUFFER_TYPE,
				INIT_DMA_DIR_TYPE,
				INIT_DMA_PRIORITY_TYPE,
				INIT_DMA_BURSTSIZE_TYPE,
				INIT_DMA_FLOWCTRL_TYPE,
				INIT_DMA_ALIGN_TYPE,
				INIT_DMA_TARGET_ADDRESS,
				tch_dma_initCfg,
				tch_dma_openStream
		},
		NULL,
		0,
		0
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


static tch_dma_handle* tch_dma_openStream(tch* api,dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg){
	/// check H/W Occupation
	if(!DMA_Manager.mtxId)
		DMA_Manager.mtxId = Mtx->create();
	if(!DMA_Manager.condvId)
		DMA_Manager.condvId = Condv->create();
	if(Mtx->lock(DMA_Manager.mtxId,timeout) != osOK)
		return NULL;
	while(DMA_Manager.occp_state & (1 << dma)){
		if(Condv->wait(DMA_Manager.condvId,DMA_Manager.mtxId,timeout) != osOK)
			return NULL;
	}
	DMA_Manager.occp_state |= (1 << dma);
	Mtx->unlock(DMA_Manager.mtxId);

	// Acquire DMA H/w
	tch_dma_descriptor* dma_desc = &DMA_HWs[dma];
	*dma_desc->_clkenr |= dma_desc->clkmsk;          // clk source is enabled
	if(pcfg == ActOnSleep)
		*dma_desc->_lpclkenr |= dma_desc->lpcklmsk;  // if dma should be awaken during sleep, lp clk is enabled
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)dma_desc->_hw;   // initializing dma H/w Started
	dmaHw->CR |= ((cfg->Ch << DMA_Ch_Pos) | (cfg->Dir << DMA_Dir_Pos) | (cfg->FlowCtrl << DMA_FlowControl_Pos));
	dmaHw->CR |= ((cfg->mBurstSize << DMA_MBurst_Pos) | (cfg->pBurstSize << DMA_PBurst_Pos));
	dmaHw->CR |= ((cfg->mAlign << DMA_MDataAlign_Pos) | (cfg->pAlign << DMA_PDataAlign_Pos));
	dmaHw->CR |= ((cfg->mInc << DMA_Minc_Pos) | (cfg->pInc << DMA_Pinc_Pos));
	dmaHw->CR |= cfg->Priority << DMA_Prior_Pos;
	dmaHw->CR |= DMA_SxCR_TCIE;

	NVIC_SetPriority(dma_desc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(dma_desc->irq);
	dma_desc->_handle = tch_dma_initHandle(dma,cfg->Ch);             // initializing dma handle object
	return (tch_dma_handle*)dma_desc->_handle;
}

static tch_dma_handle* tch_dma_initHandle(dma_t dma,uint8_t ch){
	tch_dma_handle_prototype* dma_handle = (tch_dma_handle_prototype*) malloc(sizeof(tch_dma_handle_prototype));
	dma_handle->_pix.beginXfer = tch_dma_beginXfer;
	dma_handle->_pix.close = tch_dma_close;
	dma_handle->_pix.registerEventListener = tch_dma_registerEventListener;
	dma_handle->_pix.unregisterEventListener = tch_dma_unregisterEventListener;
	dma_handle->_pix.setAddress = tch_dma_setAddress;
	dma_handle->_pix.setIncrementMode = tch_dma_setIncrementMode;
	dma_handle->ch = ch;
	dma_handle->dma = dma;
	dma_handle->status = 0;
	dma_handle->mtxId = Mtx->create();
	tch_listInit(&dma_handle->wq);
	dma_handle->dma_async = Async->create(tch_dma_trigger,dma_handle,Async->Prior.Normal);

	return (tch_dma_handle*)dma_handle;
}


static BOOL tch_dma_beginXfer(tch_dma_handle* self,uint32_t size,uint32_t timeout){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tch_dma_descriptor* dma_desc = &DMA_HWs[ins->dma];
	if(Mtx->lock(ins->mtxId,timeout) != osOK)
		return FALSE;
	tch_thread_header* header = (tch_thread_header*)Thread->self();
	uint32_t rtime = (uint32_t) header->t_to - tch_kernelCurrentSystick(); // get extra time
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)dma_desc->_hw;
	dmaHw->NDTR = size;
	ins->status |= DMA_FLAG_BUSY;
	Async->blockedstart(ins->dma_async,timeout);
	//dmaHw->CR |= DMA_SxCR_EN;
	//tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&ins->wq,rtime);
	Mtx->unlock(ins->mtxId);
	return TRUE;
}

static DECL_ASYNC_TASK(tch_dma_trigger){
	((DMA_Stream_TypeDef*)arg)->CR |= DMA_SxCR_EN;
	return osOK;
}


/*
 *
 */
static void tch_dma_setAddress(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*)self;
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)DMA_HWs[ins->dma]._hw;
	if(Mtx->lock(ins->mtxId,osWaitForever) != osOK)
		return;
	switch(targetAddress){
	case DMA_TargetAddress_Mem0:
		dmaHw->M0AR = addr;
		break;
	case DMA_TargetAddress_Mem1:
		dmaHw->M1AR = addr;
		break;
	case DMA_TargetAddress_Periph:
		dmaHw->PAR = addr;
		break;
	}
	Mtx->unlock(ins->mtxId);
}
/*
 */
static void tch_dma_registerEventListener(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tch_dma_descriptor* dma_desc = (tch_dma_descriptor*) &DMA_HWs[ins->dma];
	if(Mtx->lock(ins->mtxId,osWaitForever) != osOK)
		return;
	NVIC_DisableIRQ(dma_desc->irq);
	((DMA_Stream_TypeDef*)dma_desc)->CR |= evType;
	((DMA_Stream_TypeDef*)dma_desc)->FCR |= ((evType & (1 << 0)) << 7);
	ins->status |= evType;
	ins->listener = listener;
	NVIC_EnableIRQ(dma_desc->irq);
	Mtx->unlock(ins->mtxId);
}

/*
 *
 */
static void tch_dma_unregisterEventListener(tch_dma_handle* self){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tch_dma_descriptor* dma_desc = (tch_dma_descriptor*)&DMA_HWs[ins->dma];
	if(Mtx->lock(ins->mtxId,osWaitForever) != osOK)
		return;
	NVIC_DisableIRQ(dma_desc->irq);
	((DMA_Stream_TypeDef*)dma_desc->_hw)->CR &= ~(DMA_FLAG_EvMsk >> 1);
	((DMA_Stream_TypeDef*)dma_desc->_hw)->FCR &= ~(1 << 7);
	ins->listener = NULL;
	NVIC_EnableIRQ(dma_desc->irq);
	Mtx->unlock(ins->mtxId);
}


static void tch_dma_setIncrementMode(tch_dma_handle* self,uint8_t targetAddress,BOOL enable){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tch_dma_descriptor* dma_desc = (tch_dma_descriptor*) &DMA_HWs[ins->dma];
	if(Mtx->lock(ins->mtxId,osWaitForever) != osOK)
		return;
	switch(targetAddress){
	case DMA_TargetAddress_Mem0:
	case DMA_TargetAddress_Mem1:
		if(enable){
			((DMA_Stream_TypeDef*)dma_desc->_hw)->CR |= DMA_SxCR_MINC;
		}else{
			((DMA_Stream_TypeDef*)dma_desc->_hw)->CR &= ~DMA_SxCR_MINC;
		}
		break;
	case DMA_TargetAddress_Periph:
		if(enable){
			((DMA_Stream_TypeDef*)dma_desc->_hw)->CR |= DMA_SxCR_PINC;
		}else{
			((DMA_Stream_TypeDef*)dma_desc->_hw)->CR &= ~DMA_SxCR_PINC;
		}
		break;
	}
	Mtx->unlock(ins->mtxId);
}

static void tch_dma_close(tch_dma_handle* self){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	while(ins->status & DMA_FLAG_BUSY)
		Thread->sleep(1);
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)DMA_HWs[ins->dma]._hw;
	dmaHw->CR = 0;
	dmaHw->FCR = 0;
	Mtx->destroy(ins->mtxId);
	tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&ins->wq,0);
}


static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc){
	uint32_t isr = *dma_desc->_isr >> dma_desc->ipos;
	if(handle){
		if(isr & DMA_FLAG_EvFE){

		}
		if(isr & DMA_FLAG_EvHT){

		}
		if(isr & DMA_FLAG_EvDME){

		}
		if(isr & DMA_FLAG_EvTC){

		}
		if(isr & DMA_FLAG_EvTE){

		}
	}else{
		if(isr & DMA_FLAG_EvFE){

		}
		if(isr & DMA_FLAG_EvHT){

		}
		if(isr & DMA_FLAG_EvDME){

		}
		if(isr & DMA_FLAG_EvTC){

		}
		if(isr & DMA_FLAG_EvTE){

		}
	}

}


void DMA1_Stream0_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[0];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream1_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[1];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream2_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[2];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream3_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[3];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream4_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[4];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream5_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[5];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream6_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[6];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA1_Stream7_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[7];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream0_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[8];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream1_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[9];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream2_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[10];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream3_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[11];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream4_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[12];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream5_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[13];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream6_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[14];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}

void DMA2_Stream7_IRQHandler(void){
	tch_dma_descriptor* dhw = &DMA_HWs[15];
	tch_dma_handle_prototype* ins = dhw->_handle;
	tch_dma_handleIrq(ins,dhw);
}
