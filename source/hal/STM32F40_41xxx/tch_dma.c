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
#include "tch_halcfg.h"
#include "tch_dma.h"
#include "tch_haldesc.h"

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
#define DMA_Prior_Med            (uint8_t) 1
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


typedef struct tch_dma_manager_t {
	tch_dma_ix                 _pix;
	uint8_t                    occp_state[2];
	uint8_t                    lpoccp_state[2];
}tch_dma_manager;


static void tch_dma_initCfg(tch_dma_cfg* cfg);
static tch_dma_handle* tch_dma_openStream(dma_t dma,tch_dma_cfg* cfg,tch_pwm_def pcfg);


static BOOL tch_dma_beginXfer(tch_dma_handle* self,uint32_t size);
static void tch_dma_setAddress(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
static void tch_dma_registerEventListener(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
static void tch_dma_unregisterEventListener(tch_dma_handle* self);
static void tch_dma_setIncrementMode(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
static void tch_dma_close(tch_dma_handle* self);


__attribute__((section(".data"))) static tch_dma_manager DMA_Manager = {
		{
				tch_dma_initCfg,
				tch_dma_openStream
		},
		{0},
		{0}
};

const tch_dma_ix* Dma = (tch_dma_ix*) &DMA_Manager;



static void tch_dma_initCfg(tch_dma_cfg* cfg){
}


static tch_dma_handle* tch_dma_openStream(dma_t dma,tch_dma_cfg* cfg,tch_pwm_def pcfg){

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

