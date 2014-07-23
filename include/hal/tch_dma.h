/*
 * tch_dma.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_DMA_H_
#define TCH_DMA_H_

#include "tch_Typedef.h"

#define DMA_Str0                  (dma_t) 0
#define DMA_Str1                  (dma_t) 1
#define DMA_Str2                  (dma_t) 2
#define DMA_Str3                  (dma_t) 3
#define DMA_Str4                  (dma_t) 4
#define DMA_Str5                  (dma_t) 5
#define DMA_Str6                  (dma_t) 6
#define DMA_Str7                  (dma_t) 7
#define DMA_Str8                  (dma_t) 8
#define DMA_Str9                  (dma_t) 9
#define DMA_Str10                 (dma_t) 10
#define DMA_Str11                 (dma_t) 11
#define DMA_Str12                 (dma_t) 12
#define DMA_Str13                 (dma_t) 13
#define DMA_Str14                 (dma_t) 14
#define DMA_Str15                 (dma_t) 15
#define DMA_NOT_USED              (dma_t) -1

#define DMA_Ch_Pos                (uint8_t) 25
#define DMA_Ch0                   (uint8_t) 0
#define DMA_Ch1                   (uint8_t) 1
#define DMA_Ch2                   (uint8_t) 2
#define DMA_Ch3                   (uint8_t) 3
#define DMA_Ch4                   (uint8_t) 4
#define DMA_Ch5                   (uint8_t) 5
#define DMA_Ch6                   (uint8_t) 6
#define DMA_Ch7                   (uint8_t) 7
#define DMA_Ch_Msk                (DMA_Ch7)



typedef uint8_t dma_t;
typedef struct tch_dma_ix_t tch_dma_ix;
typedef struct tch_dma_handle_t tch_dma_handle;
typedef BOOL (*tch_dma_eventListener)(tch_dma_handle* ins,uint16_t evType);
typedef struct _dma_cfg_t tch_dma_cfg;

typedef enum {
	tch_DmaBuffType_Normal,
	tch_DmaBuffType_Double,
	tch_DmaBuffType_Circular
} tch_DmaBuffType;

typedef enum {
	tch_DmaDir_MemToMem,
	tch_DmaDir_MemToPeriph,
	tch_DmaDir_PeriphToMem
} tch_DmaDir;

typedef enum {
	tch_DmaBurstSize_1,
	tch_DmaBurstSize_2,
	tch_DmaBurstSize_4,
	tch_DmaBurstSize_8,
	tch_DmaBurstSize_16
} tch_DmaBurstSize;

typedef enum {
	tch_DmaAlign_Byte,
	tch_DmaAlign_Hword,
	tch_DmaAlign_dalignWord
} tch_DmaAlign;

typedef enum {
	tch_DmaFlowCtrl_DMA,
	tch_DmaFlowCtrl_Periph
} tch_DmaFlowCtrl;

typedef enum {
	tch_DmaPriority_Low,
	tch_DmaPriority_Mid,
	tch_DmaPriority_High,
	tch_DmaPriority_VHigh
}tch_DmaPriority;


struct tch_dma_handle_t{
	BOOL (*beginXfer)(tch_dma_handle* self,uint32_t size);
	void (*setAddress)(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
	void (*registerEventListener)(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
	void (*unregisterEventListener)(tch_dma_handle* self);
	void (*setIncrementMode)(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
	void (*close)(tch_dma_handle* self);
};

struct tch_dma_ix_t {
	void (*initCfg)(tch_dma_cfg* cfg);
	tch_dma_handle* (*openStream)(dma_t dma,tch_dma_cfg* cfg,tch_pwm_def pcfg);
};


struct _dma_cfg_t {
	uint8_t                Ch;
	tch_DmaBuffType        BufferType;
	tch_DmaDir             Dir;
	tch_DmaPriority        Priority;
	tch_DmaFlowCtrl        FlowCtrl;
	tch_DmaBurstSize       mBurstSize;
	tch_DmaBurstSize       pBurstSize;
	tch_DmaAlign           mAlign;
	tch_DmaAlign           pAlign;
	BOOL                 mInc;
	BOOL                 pInc;
};

extern const tch_dma_ix* Dma;

#endif /* TCH_DMA_H_ */
