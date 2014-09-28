/*
 * fmo_dma.h
 *
 *  Created on: 2014. 4. 14.
 *      Author: innocentevil
 */

#ifndef FMO_DMA_H_
#define FMO_DMA_H_

#include "hw_descriptor_types.h"

#define DMA1_Str0                 (dma_t) 0
#define DMA1_Str1                 (dma_t) 1
#define DMA1_Str2                 (dma_t) 2
#define DMA1_Str3                 (dma_t) 3
#define DMA1_Str4                 (dma_t) 4
#define DMA1_Str5                 (dma_t) 5
#define DMA1_Str6                 (dma_t) 6
#define DMA1_Str7                 (dma_t) 7
#define DMA2_Str0                 (dma_t) 8
#define DMA2_Str1                 (dma_t) 9
#define DMA2_Str2                 (dma_t) 10
#define DMA2_Str3                 (dma_t) 11
#define DMA2_Str4                 (dma_t) 12
#define DMA2_Str5                 (dma_t) 13
#define DMA2_Str6                 (dma_t) 14
#define DMA2_Str7                 (dma_t) 15
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


#define DMA_EVENTLISTENER(fn)           BOOL fn(tch_dma_instance* ins,uint16_t evtype)


typedef uint8_t dma_t;
typedef struct _dma_instance_t tch_dma_instance;
typedef BOOL (*tch_dma_eventListener)(tch_dma_instance* ins,uint16_t evType);
typedef struct _dma_cfg_t tch_DmaCfg;



struct _dma_instance_t{
	BOOL (*beginXfer)(tch_dma_instance* self,uint32_t size);
	void (*setAddress)(tch_dma_instance* self,uint8_t targetAddress,uint32_t addr);
	void (*registerEventListener)(tch_dma_instance* self,tch_dma_eventListener listener,uint16_t evType);
	void (*unregisterEventListener)(tch_dma_instance* self);
	void (*setIncrementMode)(tch_dma_instance* self,uint8_t targetAddress,BOOL enable);
	void (*close)(tch_dma_instance* self);
};

struct _dma_cfg_t {
	uint8_t DMA_Ch;
	uint8_t DMA_BufferMode;
	uint8_t DMA_MBurst;
	uint8_t DMA_PBurst;
	uint8_t DMA_Prior;
	uint8_t DMA_MDataAlign;
	uint8_t DMA_PDataAlign;
	uint8_t DMA_Minc;
	uint8_t DMA_Pinc;
	uint8_t DMA_Dir;
	uint8_t DMA_FlowControl;
};

void tch_lld_dma_cfginit(tch_DmaCfg* cfg);
tch_dma_instance* tch_lld_dma_init(dma_t dma,tch_DmaCfg* dma_cfg,tch_pwrMgrCfg cfg);




#endif /* FMO_DMA_H_ */
