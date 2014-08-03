
/*!
 * \defgroup TCH_DMA tch_dma
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

#include "tch_Typedef.h"

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



/*!
 *  \brief DMA Type
 *   DMA Stream Type
 */
typedef uint8_t dma_t;

/*!
 *  \brief DMA HAL Interface
 *   DMA HAL Interface which is accessible from \ref tch object
 */
typedef struct tch_dma_ix_t tch_dma_ix;

/*!
 *  \brief DMA Handle
 *   DMA Operation handle which can be obtained by \ref tch_dma_ix
 */
typedef struct tch_dma_handle_t tch_dma_handle;

/*!
 *  \brief dma event listener type
 *   DMA event Listener type. fist arg is \ref dma_handle
 */
typedef BOOL (*tch_dma_eventListener)(tch_dma_handle* ins,uint16_t evType);

/*!
 * \brief DMA Configuration type
 */
typedef struct _dma_cfg_t tch_dma_cfg;


/*!
 * \brief DMA Buffer Type
 * This Structure contains R/O Value which can be used to configure dma buffer option
 */
typedef struct _tch_dmabuffer_conf_t {
	const uint8_t Normal;            ///< Single Shot Buffer Mode
	const uint8_t Double;            ///< Double Buffer Mode, Source Memory is switched at each end of transfer
	const uint8_t Circular;          ///< Circular Buffer Mode, which means transfer begins from  buffer head at the end of transfer, thus continues loop
}tch_DmaBufferType;

/*!
 * \brief DMA Direction Type
 * This Structure contains R/O Value which can be used to configure dma direction option
 */
typedef struct _tch_dmadir_conf_t{
	const uint8_t PeriphToMem;        ///< Peripheral to Memory
	const uint8_t MemToMem;           ///< Memory to Memory
	const uint8_t MemToPeriph;        ///< Memory to Peripheral
}tch_DmaDir;


/*!
 * \brief DMA Burst-Size Type
 * This Structure contains R/O Value which can be used to configure burst size of dma transfer
 */
typedef struct _tch_dmaburst_size_t{
	const uint8_t Burst1;             ///<
	const uint8_t Burst2;
	const uint8_t Burst4;
	const uint8_t Burst8;
	const uint8_t Burst16;
}tch_DmaBurstSize;


/*!
 * \brief DMA Align Type
 * This structure contains R/O Value which can be used to configure Data Alignment of both dma source and target
 */
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

typedef struct _tch_dma_targetAddress_t{
	const uint8_t MemTarget0;
	const uint8_t MemTarget1;
	const uint8_t PeriTarget0;
	const uint8_t PeriTarget1;
}tch_DmaTargetAddress;


/*!
 * \brief DMA handle object type
 * Interface used to access control of dma H/W
 */
struct tch_dma_handle_t{
	/*!
	 * \brief Begine DMA Transfer
	 * \param \ref tch_dma_handle object which is obtained from \ref openStream
	 * \param size size of data to be transfered by DMA
	 * \return true if successful, otherwise false
	 */
	BOOL (*beginXfer)(tch_dma_handle* self,uint32_t size,uint32_t timeout);

	/*!
	 * \brief Set both source and target Address of DMA
	 * \param \ref tch_dma_handle object which is obtained from \ref openStream
	 * \param target which is among
	 */
	void (*setAddress)(tch_dma_handle* self,uint8_t targetAddress,uint32_t addr);
	void (*registerEventListener)(tch_dma_handle* self,tch_dma_eventListener listener,uint16_t evType);
	void (*unregisterEventListener)(tch_dma_handle* self);
	void (*setIncrementMode)(tch_dma_handle* self,uint8_t targetAddress,BOOL enable);
	void (*close)(tch_dma_handle* self);
};

struct tch_dma_ix_t {
	const tch_DmaBufferType     BufferType;                ///< DMA buffer type \note value can be differ from each platform H/W
	const tch_DmaDir            Dir;                       ///< Direction of DMA Stream \note Value can be differ from each platform H/W
	const tch_DmaPriority       Priority;                  ///< Priority of DMA Channel \note Value can be differ from each platform H/W
	const tch_DmaBurstSize      BurstSize;                 ///< Burst Size of DMA Stream \note Value can be differ from each platform H/W
	const tch_DmaFlowCtrl       FlowCtrl;                  ///< Flow Controller of DMA Operation. can be either DMA or Peripheral \note Value can be differ from each platform H/W
	const tch_DmaAlign          Align;                     ///< Data Alignment of DMA Source and Target \note Value can be differ from each platform H/W
	const tch_DmaTargetAddress  targetAddress;
	/*!
	 * \brief Initialize DMA Configuration
	 * \param cfg pointer of \ref tch_dma_cfg
	 * \see tch_dma_cfg
	 */
	void (*initCfg)(tch_dma_cfg* cfg);

	/*!
	 * \brief open dma stream
	 * \param dma dma stream from \ref DMA_Str0 to \ref DMA_Str15
	 * \param cfg dma configuration type. \ref tch_dma_cfg
	 * \param timeout any number of time in millisec or \ref osWaitForever
	 * \param power mode option \ref tch_pwr_def
	 * \return dma handle which allows access dma H/W
	 */
	tch_dma_handle* (*openStream)(dma_t dma,tch_dma_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg);

	/*!
	 * \brief get dma count of platform H/W
	 * \return total number of dma stream available
	 */
	int (*getDMACount)();
};


/*
 *
 */
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

/**@}*/

#endif /* TCH_DMA_H_ */
