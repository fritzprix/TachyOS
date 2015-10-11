
/*!
 * \addtogroup DMA_HAL
 * @{
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
 *
 *  \brief DMA HAL Header
 *
 *   This is HAL Interface for DMA Module. Interface itself is desinged agnostic to Platform H/W though
 *   DMA has H/W dependencies in following reason.
 *   - Number of DMA Stream and channel which can be used with specific H/W module is fixed by H/W Implementation.
 *     So if DMA HAL is freely accessible to user, it can be error prone unless user carefully investigates platform H/W datasheet
 *     and this can make user feel frustrating port their tachyos application code
 *
 *
 *
 *
 */

#ifndef TCH_DMA_H_
#define TCH_DMA_H_

#include "tch.h"

#if defined(__cplusplus)
extern "C" {
#endif

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

#define DMA_Burst_Single          (uint8_t) 0
#define DMA_Burst_Inc4            (uint8_t) 1
#define DMA_Burst_Inc8            (uint8_t) 2
#define DMA_Burst_Inc16           (uint8_t) 3

#define DMA_Dir_PeriphToMem       (uint8_t) 0
#define DMA_Dir_MemToPeriph       (uint8_t) 1
#define DMA_Dir_MemToMem          (uint8_t) 2

#define DMA_DataAlign_Byte        (uint8_t) 0
#define DMA_DataAlign_Hword       (uint8_t) 1
#define DMA_DataAlign_Word        (uint8_t) 2

#define DMA_Prior_Low            (uint8_t) 0
#define DMA_Prior_Mid            (uint8_t) 1
#define DMA_Prior_High           (uint8_t) 2
#define DMA_Prior_VHigh          (uint8_t) 4

#define DMA_FlowControl_DMA      (uint8_t) 0
#define DMA_FlowControl_Periph   (uint8_t) 1

#define DMA_BufferMode_Normal     (uint8_t) 0
#define DMA_BufferMode_Dbl        (uint8_t) 1
#define DMA_BufferMode_Cir        (uint8_t) 2

#define DMA_TargetAddress_Mem0    (uint8_t) 1
#define DMA_TargetAddress_Periph  (uint8_t) 2
#define DMA_TargetAddress_Mem1    (uint8_t) 3



typedef uint8_t dma_t;
typedef void* tch_dmaHandle;
typedef BOOL (*tch_dma_eventListener)(tch_dmaHandle ins,uint16_t evType);
typedef struct dma_cfg_t tch_DmaCfg;


typedef struct tch_dma_req_s {
	size_t      size;
	uaddr_t     MemAddr[2];
	BOOL        MemInc;
	uaddr_t     PeriphAddr[2];
	BOOL        PeriphInc;
}tch_DmaReqDef;


/*!
 * \brief DMA handle object type
 * Interface used to access control of dma H/W
 */


/*!
 *  \brief DMA HAL Interface
 *   DMA HAL Interface which is accessible from \ref tch object
 */
typedef struct tch_lld_dma {
	/*!
	 * \brief Initialize DMA Configuration
	 * \param cfg pointer of \ref tch_dma_cfg
	 * \see tch_dma_cfg
	 */
	const uint16_t count;
	void (*initCfg)(tch_DmaCfg* cfg);

	/*!
	 * \brief open dma stream
	 * \param dma dma stream from \ref DMA_Str0 to \ref DMA_Str15
	 * \param cfg dma configuration type. \ref tch_dma_cfg
	 * \param timeout any number of time in millisec or \ref osWaitForever
	 * \param power mode option \ref tch_pwr_def
	 * \return dma handle which allows access dma H/W
	 */
	void (*initReq)(tch_DmaReqDef* attr,uaddr_t maddr,uaddr_t paddr,size_t size);
	tch_dmaHandle (*allocate)(const tch* api,dma_t dma,tch_DmaCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg);
	uint32_t (*beginXfer)(tch_dmaHandle self,tch_DmaReqDef* req,uint32_t timeout,tchStatus* result);
	tchStatus (*freeDma)(tch_dmaHandle handle);

}tch_lld_dma;


/*
 *
 */
struct dma_cfg_t {
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


#if defined(__cplusplus)
}
#endif

/**@}*/

#endif /* TCH_DMA_H_ */
