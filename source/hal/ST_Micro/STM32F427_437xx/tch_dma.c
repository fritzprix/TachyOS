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


#define DMA_MAX_ERR_CNT           ((uint8_t)  2)

#define SET_SAFE_RETURN();        SAFE_RETURN:
#define RETURN_SAFE()             goto SAFE_RETURN;


#define DMA_Ch_Pos                (uint8_t) 25 ///< DMA Channel bit position
#define DMA_Ch_Msk                (7)


#define DMA_MBurst_Pos            (uint8_t) 23
#define DMA_PBurst_Pos            (uint8_t) 21

#define DMA_Burst_Msk             (DMA_Burst_Single |\
		                           DMA_Burst_Inc4|\
		                           DMA_Burst_Inc8|\
		                           DMA_Burst_Inc16)

#define DMA_Minc_Pos              (uint8_t) 10
#define DMA_Pinc_Pos              (uint8_t) 9



#define DMA_Dir_Pos               (uint8_t) 6

#define DMA_Dir_Msk               (DMA_Dir_PeriphToMem |\
		                           DMA_Dir_MemToPeriph |\
		                           DMA_Dir_MemToMem)

#define DMA_MDataAlign_Pos        (uint8_t) 13
#define DMA_PDataAlign_Pos        (uint8_t) 11

#define DMA_DataAlign_Msk         (DMA_DataAlign_Byte  |\
		                           DMA_DataAlign_Hword |\
		                           DMA_DataAlign_Word)

#define DMA_Prior_Pos            (uint8_t) 16

#define DMA_FlowControl_Pos      (uint8_t) 5




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




typedef struct tch_dma_req_t {
	void* self;
	tch_DmaReqDef* attr;
}tch_DmaReqArgs;

typedef struct tch_dma_manager_t {
	tch_lld_dma                 _pix;
	tch_mtxId                   mtxId;
	tch_condvId                 condvId;
	uint16_t                    occp_state;
	uint16_t                    lpoccp_state;
}tch_dma_manager;



#define TCH_DMA_CLASS_KEY             ((uint16_t) 0x3D01)
#define TCH_DMA_BUSY                  ((uint32_t) 1 << 16)

#define DMA_SET_BUSY(ins)\
	do{\
		((tch_dma_handle_prototype*) ins)->status |= TCH_DMA_BUSY;\
	}while(0)

#define DMA_CLR_BUSY(ins)\
	do{\
		((tch_dma_handle_prototype*) ins)->status &= ~TCH_DMA_BUSY;\
	}while(0)

#define DMA_IS_BUSY(ins)              (((tch_dma_handle_prototype*) ins)->status & TCH_DMA_BUSY) == TCH_DMA_BUSY






typedef struct tch_dma_handle_prototype_t{
	tch_uobjDestr               destr;
	const tch*                  env;
	dma_t                       dma;
	uint8_t                     ch;
	tch_mtxId                   mtxId;
	tch_condvId                 condv;
	tch_dma_eventListener       listener;
	uint32_t                    status;
	tch_msgqId                  dma_mq;
}tch_dma_handle_prototype;





static void tch_dma_initCfg(tch_DmaCfg* cfg);
static void tch_dma_initReq(tch_DmaReqDef* attr,uaddr_t maddr,uaddr_t paddr,size_t size);
static tch_DmaHandle tch_dma_openStream(const tch* sys,dma_t dma,tch_DmaCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg);
static uint32_t tch_dma_beginXfer(tch_DmaHandle self,tch_DmaReqDef* attr,uint32_t timeout,tchStatus* result);

static tchStatus tch_dma_close(tch_DmaHandle self);
static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc);


/*              dma private function list                */
static inline void tch_dmaValidate(tch_dma_handle_prototype* _handle);
static inline void tch_dmaInvalidate(tch_dma_handle_prototype* _handle);
static inline BOOL tch_dmaIsValid(tch_dma_handle_prototype* _handle);
static BOOL tch_dmaSetDmaAttr(void* _dmaHw,tch_DmaReqDef* attr);


__attribute__((section(".data"))) static tch_dma_manager DMA_StaticInstance = {
		{
				16,
				tch_dma_initCfg,
				tch_dma_initReq,
				tch_dma_openStream,
				tch_dma_beginXfer,
				tch_dma_close
		},
		NULL,
		NULL,
		0,
		0
};


tch_lld_dma* tch_dmaHalInit(const tch* env){
	if(DMA_StaticInstance.mtxId || DMA_StaticInstance.condvId)
		return NULL;
	if(!env)
		return NULL;
	DMA_StaticInstance.mtxId = env->Mtx->create();
	DMA_StaticInstance.condvId = env->Condv->create();
	return (tch_lld_dma*) &DMA_StaticInstance;
}

static void tch_dma_initCfg(tch_DmaCfg* cfg){
	cfg->BufferType = DMA_BufferMode_Normal;
	cfg->Ch = 0;
	cfg->Dir = DMA_Dir_MemToPeriph;
	cfg->FlowCtrl = DMA_FlowControl_DMA;
	cfg->Priority = DMA_Prior_Mid;
	cfg->mAlign = DMA_DataAlign_Byte;
	cfg->pAlign = DMA_DataAlign_Byte;
	cfg->mInc = FALSE;
	cfg->pInc = FALSE;
	cfg->mBurstSize = DMA_Burst_Single;
	cfg->pBurstSize = DMA_Burst_Single;
}

static void tch_dma_initReq(tch_DmaReqDef* attr,uaddr_t maddr,uaddr_t paddr,size_t size){
	attr->MemAddr[0] = maddr;
	attr->PeriphAddr[0] = paddr;
	attr->MemInc = TRUE;
	attr->PeriphInc = FALSE;
	attr->size = size;
}




static tch_DmaHandle tch_dma_openStream(const tch* env,dma_t dma,tch_DmaCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg){
	tch_dma_handle_prototype* ins;
	if(dma == DMA_NOT_USED)
		return NULL;
	if(Mtx->lock(DMA_StaticInstance.mtxId,timeout) != tchOK)
		return NULL;
	while(DMA_StaticInstance.occp_state & (1 << dma)){
		if(Condv->wait(DMA_StaticInstance.condvId,DMA_StaticInstance.mtxId,timeout) != tchOK)
			return NULL;
	}
	DMA_StaticInstance.occp_state |= (1 << dma);
	Mtx->unlock(DMA_StaticInstance.mtxId);

	ins = (tch_dma_handle_prototype*) env->Mem->alloc(sizeof(tch_dma_handle_prototype));

	// Acquire DMA H/w
	tch_dma_descriptor* dma_desc = &DMA_HWs[dma];
	*dma_desc->_clkenr |= dma_desc->clkmsk;          // clk source is enabled
	if(pcfg == ActOnSleep){
		DMA_StaticInstance.lpoccp_state |= (1 << dma);
		*dma_desc->_lpclkenr |= dma_desc->lpcklmsk;  // if dma should be awaken during sleep, lp clk is enabled
	}

	ins->destr = (tch_uobjDestr) tch_dma_close;
	ins->env = env;
	ins->ch = cfg->Ch;
	ins->dma = dma;
	ins->status = 0;
	ins->mtxId = env->Mtx->create();
	ins->condv = env->Condv->create();
	ins->dma_mq = env->MsgQ->create(1);
	ins->listener = NULL;

	dma_desc->_handle = ins;             // initializing dma handle object

	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)dma_desc->_hw;   // initializing dma H/w Started
	dmaHw->CR |= ((cfg->Ch << DMA_Ch_Pos) | (cfg->Dir << DMA_Dir_Pos) | (cfg->FlowCtrl << DMA_FlowControl_Pos));
	dmaHw->CR |= ((cfg->mBurstSize << DMA_MBurst_Pos) | (cfg->pBurstSize << DMA_PBurst_Pos));
	dmaHw->CR |= ((cfg->mAlign << DMA_MDataAlign_Pos) | (cfg->pAlign << DMA_PDataAlign_Pos));
	dmaHw->CR |= ((cfg->mInc << DMA_Minc_Pos) | (cfg->pInc << DMA_Pinc_Pos));
	dmaHw->CR |= cfg->Priority << DMA_Prior_Pos;
	dmaHw->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE | DMA_SxCR_DMEIE;
/*
	NVIC_SetPriority(dma_desc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(dma_desc->irq);*/
	tch_kernel_enableInterrupt(dma_desc->irq,HANDLER_NORMAL_PRIOR);
	__DMB();
	__ISB();
	tch_dmaValidate(dma_desc->_handle);
	return (tch_DmaHandle*)dma_desc->_handle;
}




static uint32_t tch_dma_beginXfer(tch_DmaHandle self,tch_DmaReqDef* attr,uint32_t timeout,tchStatus* result){
	if(!self)
		return -1;
	if(!tch_dmaIsValid((tch_dma_handle_prototype*)self))
		return -1;

	uint32_t residue = 0;
	uint8_t err_cnt = 0;
	tchEvent evt;
	evt.status = tchOK;
	if(!result)
		result = &evt.status;
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tchStatus lresult = tchOK;
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*)DMA_HWs[ins->dma]._hw;

	if((*result = ins->env->Mtx->lock(ins->mtxId,timeout)) != tchOK)
		return -1;
	while(DMA_IS_BUSY(ins)){
		if((*result = ins->env->Condv->wait(ins->condv,ins->mtxId,timeout)) != tchOK)
			return -1;
	}
	DMA_SET_BUSY(ins);
	ins->env->Mtx->unlock(ins->mtxId);
	if(!result)
		result = &lresult;

	tch_dmaSetDmaAttr(dmaHw,attr);  // configure DMA
	__ISB();
	__DMB();
	dmaHw->CR |= DMA_SxCR_EN;       // trigger dma tranfer in system task thread

	if(timeout){
		evt = ins->env->MsgQ->get(ins->dma_mq,timeout);
		*result = evt.status;
		if(evt.status != tchEventMessage){
			residue = dmaHw->NDTR;
			RETURN_SAFE();
		}
	}


	SET_SAFE_RETURN();
	ins->env->Mtx->lock(ins->mtxId,timeout);   // lock mutex for condition variable operation
	dmaHw->CR &= ~DMA_SxCR_EN;
	DMA_CLR_BUSY(ins);                 // clear DMA Busy and wake waiting thread
	ins->env->Condv->wakeAll(ins->condv);
	ins->env->Mtx->unlock(ins->mtxId); // unlock mutex

	return 0;
}

static BOOL tch_dmaSetDmaAttr(void* _dmaHw,tch_DmaReqDef* attr){
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*) _dmaHw;
	if(!attr->size)
		return FALSE;
	dmaHw->NDTR = attr->size;
	dmaHw->M0AR = (uint32_t)attr->MemAddr[0];
	dmaHw->M1AR = (uint32_t)attr->MemAddr[1];
	dmaHw->PAR = (uint32_t)attr->PeriphAddr[0];

	if(attr->MemInc)
		dmaHw->CR |= DMA_SxCR_MINC;
	else
		dmaHw->CR &= ~DMA_SxCR_MINC;

	if(attr->PeriphInc)
		dmaHw->CR |= DMA_SxCR_PINC;
	else
		dmaHw->CR &= ~DMA_SxCR_PINC;
	return TRUE;

}

static tchStatus tch_dma_close(tch_DmaHandle self){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tchStatus result = tchOK;
	if(!tch_dmaIsValid(ins))
		return tchErrorResource;
	const tch* env = ins->env;
	DMA_Stream_TypeDef* dmaHw = (DMA_Stream_TypeDef*) DMA_HWs[ins->dma]._hw;
	// wait until dma is busy
	if((result = env->Mtx->lock(ins->mtxId,tchWaitForever)) != tchOK)
		return result;
	while(DMA_IS_BUSY(ins)){
		if((result = env->Condv->wait(ins->condv,ins->mtxId,tchWaitForever)) != tchOK)
			return result;
	}
	tch_dmaInvalidate(ins);
//	api->Async->destroy(ins->dma_async);
	env->Mtx->destroy(ins->mtxId);
	env->MsgQ->destroy(ins->dma_mq);
	env->Condv->destroy(ins->condv);


	if((result = env->Mtx->lock(DMA_StaticInstance.mtxId,tchWaitForever)) != tchOK)
		return result;
	tch_dma_descriptor* dma_desc = &DMA_HWs[ins->dma];
	DMA_StaticInstance.lpoccp_state &= ~(1 << ins->dma);
	DMA_StaticInstance.occp_state &= ~(1 << ins->dma);

	//check unused dma stream

	*dma_desc->_clkenr &= ~dma_desc->clkmsk;
	*dma_desc->_lpclkenr &= ~dma_desc->lpcklmsk;
	NVIC_DisableIRQ(dma_desc->irq);

	DMA_HWs[ins->dma]._handle = NULL;

	env->Condv->wakeAll(DMA_StaticInstance.condvId);     // notify dma has been released
	env->Mtx->unlock(DMA_StaticInstance.mtxId);          // unlock DMA Singleton

	env->Mem->free(ins);
	return result;
}


static inline void tch_dmaValidate(tch_dma_handle_prototype* _handle){
	_handle->status = (((uint32_t) _handle) & 0xFFFF) ^ TCH_DMA_CLASS_KEY;
}

static inline void tch_dmaInvalidate(tch_dma_handle_prototype* _handle){
	_handle->status &= ~0xFFFF;
}

static inline BOOL tch_dmaIsValid(tch_dma_handle_prototype* _handle){
	return (_handle->status & 0xFFFF) == ((((uint32_t) _handle) & 0xFFFF) ^ TCH_DMA_CLASS_KEY);
}

static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc){
	uint32_t isr = (*dma_desc->_isr >> dma_desc->ipos) & 63;
	uint32_t isrclr = *dma_desc->_isr;
	if(!handle){
		*dma_desc->_isr = isrclr;
		return FALSE;                     // return
	}
	if(handle->listener){
		if(!handle->listener((tch_DmaHandle)handle,isr))
			return FALSE;
	}
	const tch* env = handle->env;
	if(isr & DMA_FLAG_EvTC){
		env->MsgQ->put(handle->dma_mq,tchOK,0);
		*dma_desc->_icr = isrclr;
		return TRUE;
	}else{
		env->MsgQ->put(handle->dma_mq,tchErrorOS,0);
		*dma_desc->_icr = isrclr;
		return FALSE;
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
