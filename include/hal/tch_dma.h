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



typedef struct _tch_dmabuffer_conf_t {
	const uint8_t Normal;
	const uint8_t Double;
	const uint8_t Circular;
}tch_DmaBufferType;

typedef struct _tch_dmadir_conf_t{
	const uint8_t PeriphToMem;
	const uint8_t MemToMem;
	const uint8_t MemToPeriph;
}tch_DmaDir;

typedef struct _tch_dmaburst_size_t{
	const uint8_t Burst1;
	const uint8_t Burst2;
	const uint8_t Burst4;
	const uint8_t Burst8;
	const uint8_t Burst16;
}tch_DmaBurstSize;

typedef struct _tch_dma_align_t{
	const uint8_t Byte;
	const uint8_t Hword;
	const uint8_t word;
}tch_DmaAlign;

typedef struct _tch_dma_priority_t{
	const uint8_t VeryHigh;
	const uint8_t High;
	const uint8_t Normal;
	const uint8_t Low;
}tch_DmaPriority;

typedef struct _tch_dma_flowctrl_t{
	const uint8_t Periph;
	const uint8_t DMA;
}tch_DmaFlowCtrl;



struct tch_dma_handle_t{
	BOOL (*beginXfer)(tch_dma_handle* self,uint32_t size);
	void (*setAddress)(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
	void (*registerEventListener)(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
	void (*unregisterEventListener)(tch_dma_handle* self);
	void (*setIncrementMode)(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
	void (*close)(tch_dma_handle* self);
};

struct tch_dma_ix_t {
	const tch_DmaBufferType   BufferType;
	const tch_DmaDir          Dir;
	const tch_DmaPriority     Priority;
	const tch_DmaBurstSize    BurstSize;
	const tch_DmaFlowCtrl     FlowCtrl;
	const tch_DmaAlign        Align;
	void (*initCfg)(tch_dma_cfg* cfg);
	tch_dma_handle* (*openStream)(dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg);
};


struct _dma_cfg_t {
	uint8_t                Ch;
	uint8_t                BufferType;
	uint8_t                Dir;
	uint8_t                Priority;
	uint8_t                FlowCtrl;
	uint8_t                mBurstSize;
	uint8_t                pBurstSize;
	uint8_t                mAlign;
	uint8_t                pAlign;
	BOOL                   mInc;
	BOOL                   pInc;
};

extern const tch_dma_ix* Dma;

#endif /* TCH_DMA_H_ */
