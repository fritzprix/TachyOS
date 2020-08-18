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
#include "tch_dma.h"

#include "kernel/tch_kmod.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_interrupt.h"


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

#ifndef DMA_CLASS_KEY
#define DMA_CLASS_KEY   	          ((uint16_t) 0x3D01)
#endif


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
	tch_kobjDestr               destr;
	const tch_core_api_t*                  api;
	dma_t                       dma;
	uint8_t                     ch;
	tch_mtxCb                   lock;
	tch_condvCb                 condWait;
	tch_dma_eventListener       listener;
	uint32_t                    status;
	tch_msgqId                  dma_mq;
}tch_dma_handle_prototype;



/**
 *  User Accessible Stubs
 */
__USER_API__ static void tch_dma_initConfig(tch_DmaCfg* cfg);
__USER_API__ static void tch_dma_initReq(tch_DmaReqDef* attr,uwaddr_t maddr,uwaddr_t paddr,size_t size,uint8_t dir);
__USER_API__ static tch_dmaHandle tch_dma_openStream(const tch_core_api_t* sys,dma_t dma,tch_DmaCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg);
__USER_API__ static uint32_t tch_dma_beginXferSync(tch_dmaHandle self,tch_DmaReqDef* req,uint32_t timeout,tchStatus* result);
__USER_API__ static tchStatus tch_dma_beginXferAsync(tch_dmaHandle self, tch_DmaReqDef* req);
__USER_API__ static tchStatus tch_dma_waitComplete(tch_dmaHandle self, uint32_t timeout);
__USER_API__ static tchStatus tch_dma_close(tch_dmaHandle self);

static int tch_dma_init(void);
static void tch_dma_exit(void);

static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc);
static inline void tch_dma_validate(tch_dma_handle_prototype* _handle);
static inline void tch_dma_invalidate(tch_dma_handle_prototype* _handle);
static inline BOOL tch_dma_isValid(tch_dma_handle_prototype* _handle);
static BOOL tch_dma_setDmaRequest(void* _dmaHw,tch_DmaReqDef* attr);


__USER_RODATA__ tch_hal_module_dma_t DMA_Ops = {
	.count = MFEATURE_DMA,
	.initCfg = tch_dma_initConfig,
	.initReq = tch_dma_initReq,
	.allocate = tch_dma_openStream,
	.beginXferAsync = tch_dma_beginXferAsync,
	.beginXferSync = tch_dma_beginXferSync,
	.waitComplete = tch_dma_waitComplete,
	.freeDma = tch_dma_close
};

static tch_mtxCb mutex;
static tch_condvCb condvb;
static uint16_t occp_state = 0;
static uint16_t lpoccp_state = 0;


static int tch_dma_init(void){

	tch_mutexInit(&mutex);
	tch_condvInit(&condvb);
	tch_kmod_register(MODULE_TYPE_DMA,DMA_CLASS_KEY,&DMA_Ops,TRUE);
	return TRUE;
}

static void tch_dma_exit(void){
	tch_kmod_unregister(MODULE_TYPE_DMA,DMA_CLASS_KEY);
	tch_mutexDeinit(&mutex);
	tch_condvInit(&condvb);
}

MODULE_INIT(tch_dma_init);
MODULE_EXIT(tch_dma_exit);

__USER_API__ static void tch_dma_initConfig(tch_DmaCfg* cfg){
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

__USER_API__ static void tch_dma_initReq(tch_DmaReqDef* attr,uwaddr_t maddr,uwaddr_t paddr,size_t size,uint8_t dir){
	attr->MemAddr[0] = maddr;
	attr->PeriphAddr[0] = paddr;
	attr->MemInc = TRUE;
	attr->PeriphInc = FALSE;
	attr->size = size;
	attr->Dir = dir;
}

__USER_API__ static tch_dmaHandle tch_dma_openStream(const tch_core_api_t* env,dma_t dma,tch_DmaCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg){
	tch_dma_handle_prototype* ins;
	if(dma == DMA_NOT_USED)
	{
		return NULL;
	}
	if(Mtx->lock(&mutex,timeout) != tchOK)
	{
		return NULL;
	}
	while(occp_state & (1 << dma)){
		if(Condv->wait(&condvb,&mutex,timeout) != tchOK) 
		{
			return NULL;
		}
	}
	occp_state |= (1 << dma);
	Mtx->unlock(&mutex);

	ins = (tch_dma_handle_prototype*) env->Mem->alloc(sizeof(tch_dma_handle_prototype));

	tch_dma_descriptor* dma_desc = &DMA_HWs[dma];
	*dma_desc->_clkenr |= dma_desc->clkmsk;          // clk source is enabled

	if(pcfg == ActOnSleep && dma_desc->_lpclkenr)
	{
		lpoccp_state |= (1 << dma);
		*dma_desc->_lpclkenr |= dma_desc->lpcklmsk;  // if dma should be awaken during sleep, lp clk is enabled
	}

	ins->destr = (tch_kobjDestr) tch_dma_close;
	ins->api = env;
	ins->ch = cfg->Ch;
	ins->dma = dma;
	ins->status = 0;
	tch_mutexInit(&ins->lock);
	tch_condvInit(&ins->condWait);
	ins->dma_mq = env->MsgQ->create(1);
	ins->listener = NULL;

	dma_desc->_handle = ins;             // initializing dma handle object

	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) dma_desc->_hw;   // initializing dma H/w Started
	dmaHw->CCR |= ((cfg->Ch << DMA_Ch_Pos) | (cfg->Dir << DMA_Dir_Pos) | (cfg->FlowCtrl << DMA_FlowControl_Pos));
	dmaHw->CCR |= ((cfg->mBurstSize << DMA_MBurst_Pos) | (cfg->pBurstSize << DMA_PBurst_Pos));
	dmaHw->CCR |= ((cfg->mAlign << DMA_MDataAlign_Pos) | (cfg->pAlign << DMA_PDataAlign_Pos));
	dmaHw->CCR |= ((cfg->mInc << DMA_Minc_Pos) | (cfg->pInc << DMA_Pinc_Pos));
	dmaHw->CCR |= cfg->Priority << DMA_Prior_Pos;
	dmaHw->CCR |= DMA_CCRx_TCIE | DMA_CCRx_TEIE;

	// TODO : put dummy handler because isr remap is not supported currently, however, consider supporting it
	tch_enableInterrupt(dma_desc->irq,PRIORITY_2,NULL);
	__DMB();
	__ISB();
	tch_dma_validate(dma_desc->_handle);
	return (tch_dmaHandle*)dma_desc->_handle;
}




__USER_API__ static uint32_t tch_dma_beginXferSync(tch_dmaHandle self,tch_DmaReqDef* req,uint32_t timeout,tchStatus* result)
{
	if(!self || !req)
		return 0;
	if(!tch_dma_isValid((tch_dma_handle_prototype*)self))
		return 0;

	uint32_t residue = 0;
	uint8_t err_cnt = 0;
	tchEvent evt;
	evt.status = tchOK;
	if(!result)
		result = &evt.status;
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tchStatus lresult = tchOK;
	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) DMA_HWs[ins->dma]._hw;


	if((*result = ins->api->Mtx->lock(&ins->lock,timeout)) != tchOK)
		return 0;
	while(DMA_IS_BUSY(ins)){
		if((*result = ins->api->Condv->wait(&ins->condWait,&ins->lock,timeout)) != tchOK)
			return 0;
	}
	DMA_SET_BUSY(ins);
	ins->api->Mtx->unlock(&ins->lock);
	if(!result)
		result = &lresult;

	ins->api->MsgQ->reset(ins->dma_mq);

	tch_dma_setDmaRequest(dmaHw,req);  // configure DMA
	dmaHw->CCR |= DMA_CCRx_EN;       // trigger dma tranfer in system task thread

	evt = ins->api->MsgQ->get(ins->dma_mq,timeout);
	*result = evt.status;
	if(evt.status != tchEventMessage){
		residue = dmaHw->CNDTR;
	}

	dmaHw->CCR &= ~DMA_CCRx_EN;

	ins->api->Mtx->lock(&ins->lock,timeout);   // lock mutex for condition variable operation
	DMA_CLR_BUSY(ins);                 // clear DMA Busy and wake waiting thread
	ins->api->Condv->wakeAll(&ins->condWait);
	ins->api->Mtx->unlock(&ins->lock); // unlock mutex

	return residue;
}

__USER_API__ static tchStatus tch_dma_beginXferAsync(tch_dmaHandle self, tch_DmaReqDef* req)
{
	if(!self || !req)
		return tchErrorParameter;
	if(!tch_dma_isValid(self))
		return tchErrorParameter;

	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) DMA_HWs[ins->dma]._hw;
	tchStatus res;

	if((res = ins->api->Mtx->lock(&ins->lock,tchWaitForever)) != tchOK)
	{
		return res;
	}

	while(DMA_IS_BUSY(ins))
	{
		if((res = ins->api->Condv->wait(&ins->condWait,&ins->lock, tchWaitForever)) != tchOK)
		{
			return res;
		}
	}
	DMA_SET_BUSY(ins);

	if((res = ins->api->Mtx->unlock(&ins->lock)) != tchOK)
	{
		return res;
	}

	ins->api->MsgQ->reset(ins->dma_mq);

	tch_dma_setDmaRequest(dmaHw, req);
	dmaHw->CCR |= DMA_CCRx_EN;       // trigger dma tranfer in system task thread

	ins->api->Mtx->lock(&ins->lock, tchWaitForever);
	DMA_CLR_BUSY(ins);
	ins->api->Condv->wake(&ins->condWait);
	ins->api->Mtx->unlock(&ins->lock);
	return res;
}

__USER_API__ static tchStatus tch_dma_waitComplete(tch_dmaHandle self, uint32_t timeout)
{
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_dma_isValid(self))
	{
		return tchErrorParameter;
	}


	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) DMA_HWs[ins->dma]._hw;
	tchEvent evt;

	if((evt.status = ins->api->Mtx->lock(&ins->lock,timeout)) != tchOK)
	{
		return evt.status;
	}
	while(DMA_IS_BUSY(ins))
	{
		if((evt.status = ins->api->Condv->wait(&ins->condWait, &ins->lock, timeout)) != tchOK)
		{
			return evt.status;
		}
	}
	DMA_SET_BUSY(ins);

	if((evt.status = ins->api->Mtx->unlock(&ins->lock)) != tchOK)
	{
		return evt.status;
	}

	evt = ins->api->MsgQ->get(ins->dma_mq, timeout);
	dmaHw->CCR &= ~DMA_CCRx_EN;


	ins->api->Mtx->lock(&ins->lock, timeout);
	DMA_CLR_BUSY(ins);
	ins->api->Condv->wake(&ins->condWait);
	ins->api->Mtx->unlock(&ins->lock);
	return evt.value.v;

}




__USER_API__ static tchStatus tch_dma_close(tch_dmaHandle self){
	tch_dma_handle_prototype* ins = (tch_dma_handle_prototype*) self;
	tchStatus result = tchOK;
	if(!tch_dma_isValid(ins))
	{
		return tchErrorResource;
	}

	const tch_core_api_t* env = ins->api;
	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) DMA_HWs[ins->dma]._hw;
	// wait until dma is busy
	if((result = env->Mtx->lock(&ins->lock,tchWaitForever)) != tchOK) 
	{
		return result;
	}
	while(DMA_IS_BUSY(ins))
	{
		if((result = env->Condv->wait(&ins->condWait,&ins->lock,tchWaitForever)) != tchOK)
		{
			return result;
		}
	}
	tch_dma_invalidate(ins);
	tch_mutexDeinit(&ins->lock);
	env->MsgQ->destroy(ins->dma_mq);
	tch_condvDeinit(&ins->condWait);

	if((result = env->Mtx->lock(&mutex,tchWaitForever)) != tchOK)
		return result;
	tch_dma_descriptor* dma_desc = &DMA_HWs[ins->dma];
	lpoccp_state &= ~(1 << ins->dma);
	occp_state &= ~(1 << ins->dma);

	//check unused dma stream

	*dma_desc->_clkenr &= ~dma_desc->clkmsk;
	if(dma_desc->_lpclkenr) 
	{
		*dma_desc->_lpclkenr &= ~dma_desc->lpcklmsk;
	}

	tch_disableInterrupt(dma_desc->irq);

	DMA_HWs[ins->dma]._handle = NULL;

	env->Condv->wakeAll(&condvb);     // notify dma has been released
	env->Mtx->unlock(&mutex);          // unlock DMA Singleton

	env->Mem->free(ins);
	return result;
}



static inline void tch_dma_validate(tch_dma_handle_prototype* _handle){
	_handle->status = (((uint32_t) _handle) & 0xFFFF) ^ DMA_CLASS_KEY;
}

static inline void tch_dma_invalidate(tch_dma_handle_prototype* _handle){
	_handle->status &= ~0xFFFF;
}

static inline BOOL tch_dma_isValid(tch_dma_handle_prototype* _handle){
	return (_handle->status & 0xFFFF) == ((((uint32_t) _handle) & 0xFFFF) ^ DMA_CLASS_KEY);
}


static BOOL tch_dma_setDmaRequest(void* _dmaHw,tch_DmaReqDef* attr){
	DMA_Channel_TypeDef* dmaHw = (DMA_Channel_TypeDef*) _dmaHw;
	dmaHw->CNDTR = attr->size;
	dmaHw->CMAR = ((uint32_t) attr->MemAddr[0] | ((uint32_t) attr->MemAddr[1] << 16));
	dmaHw->CPAR = (uint32_t) attr->PeriphAddr[0];

	if(attr->MemInc)  
	{
		dmaHw->CCR |= DMA_CCRx_MINC;
	}
	else 
	{
		dmaHw->CCR &= ~DMA_CCRx_MINC;
	} 
	dmaHw->CCR = ((dmaHw->CCR & ~DMA_CCRx_DIR) | (attr->Dir << 4));

	if(attr->PeriphInc) 
	{
		dmaHw->CCR |= DMA_CCRx_PINC;
	}
	else 
	{
		dmaHw->CCR &= ~DMA_CCRx_PINC;
	}

	__ISB();
	__DMB();

	return TRUE;

}

static BOOL tch_dma_handleIrq(tch_dma_handle_prototype* handle,tch_dma_descriptor* dma_desc){
	uint32_t isr = (*dma_desc->_isr >> dma_desc->ipos) & 63;
	uint32_t isrclr = *dma_desc->_isr;
	if(!handle){
		*dma_desc->_isr = isrclr;
		return FALSE;                     // return
	}
	if(handle->listener){
		if(!handle->listener((tch_dmaHandle)handle,isr))
			return FALSE;
	}
	const tch_core_api_t* env = handle->api;
	if(isr & DMA_FLAG_EvTC){
		*dma_desc->_icr = isrclr;
		env->MsgQ->put(handle->dma_mq,tchOK,0);
		return TRUE;
	}else{
		*dma_desc->_icr = isrclr;
		env->MsgQ->put(handle->dma_mq,tchErrorOS,0);
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
