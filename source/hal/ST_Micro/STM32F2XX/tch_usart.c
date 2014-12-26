/*
 * tch_usart.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "tch_hal.h"
#include "tch_halinit.h"
#include "tch_dma.h"
#include "tch_halcfg.h"
#include "tch_port.h"


#define TCH_UART_CLASS_KEY     ((uint16_t) 0x3D02)

#define TCH_URX_QSZ                         ((size_t) 10)

#define USART_Parity_Pos_CR1                (uint8_t) (9)

#define USART_StopBit_Pos_CR2               (uint8_t) (12)


#define UART_RXBUSY                         ((uint32_t) 0x10000)
#define UART_TXBUSY                         ((uint32_t) 0x20000)


#define UART_SET_RXBUSY(ins)\
	do{\
		((tch_UartHandlePrototype*) ins)->status |= UART_RXBUSY;\
	}while(0)

#define UART_CLR_RXBUSY(ins)\
	do{\
		((tch_UartHandlePrototype*) ins)->status &= ~UART_RXBUSY;\
	}while(0)

#define UART_IS_RXBUSY(ins)         ((tch_UartHandlePrototype*) ins)->status & UART_RXBUSY


#define UART_SET_TXBUSY(ins)\
	do{\
		((tch_UartHandlePrototype*) ins)->status |= UART_TXBUSY;\
	}while(0)

#define UART_CLR_TXBUSY(ins)\
	do{\
		((tch_UartHandlePrototype*) ins)->status &= ~UART_TXBUSY;\
	}while(0)

#define UART_IS_TXBUSY(ins)          ((tch_UartHandlePrototype*) ins)->status & UART_TXBUSY



typedef struct tch_lld_usart_handle_prototype_t {
	tch_UartHandle                    pix;
	uart_t                            pno;
	union {
		tch_DmaHandle txDma;
		tch_msgqId txQ;
	}txCh;
	union {
		tch_DmaHandle rxDma;
		tch_msgqId rxQ;
	} rxCh;
	tch_GpioHandle*                   ioHandle;
	tch_mtxId                         rxMtx;
	tch_condvId                       rxCondv;
	tch_mtxId                         txMtx;
	tch_condvId                       txCondv;
	uint32_t                          status;
	const tch*                        env;
}tch_UartHandlePrototype;

typedef struct tch_lld_uart_prototype {
	tch_lld_usart                     pix;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint16_t                          occp_state;
	uint16_t                          lpoccp_state;
}tch_lld_uart_prototype;

static tch_UartHandle* tch_uartOpen(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
static tchStatus tch_uartClose(tch_UartHandle* handle);

static tchStatus tch_uartPutc(tch_UartHandle* handle,uint8_t c);
static tchStatus tch_uartGetc(tch_UartHandle* handle,uint8_t* rc,uint32_t timeout);
static tchStatus tch_uartWrite(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
static tchStatus tch_uartRead(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);

static tchStatus tch_uartWriteDma(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
static tchStatus tch_uartReadDma(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);


static BOOL tch_uartInterruptHandler(tch_UartHandlePrototype* _handle,void* _hw);
static inline void tch_uartValidate(tch_UartHandlePrototype* _handle);
static inline void tch_uartInvalidate(tch_UartHandlePrototype* _handle);
static inline BOOL tch_uartIsValid(tch_UartHandlePrototype* _handle);

__attribute__((section(".data"))) static tch_lld_uart_prototype UART_StaticInstance = {
		{
				MFEATURE_UART,
				tch_uartOpen
		},
		NULL,
		NULL,
		0,
		0
};



const tch_lld_usart* tch_usart_instance = (const tch_lld_usart*) &UART_StaticInstance;

static tch_UartHandle* tch_uartOpen(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	tch_UartHandlePrototype* uins = NULL;
	tch_GpioCfg iocfg;
	uint32_t io_msk = 0;
	uint32_t umskb = 1 << port;

	if(port >= MFEATURE_UART)    // if requested channel is larger than uart channel count
		return NULL;                    // return null
	if(!UART_StaticInstance.mtx)
		UART_StaticInstance.mtx = env->Mtx->create();
	if(!UART_StaticInstance.condv)
		UART_StaticInstance.condv = env->Condv->create();
	if(tch_port_isISR())
		return NULL;


	if(env->Mtx->lock(UART_StaticInstance.mtx,timeout) != osOK)
		return NULL;
	while(UART_StaticInstance.occp_state & umskb){
		if(env->Condv->wait(UART_StaticInstance.condv,UART_StaticInstance.mtx,timeout) != osOK)
			return NULL;
	}
	UART_StaticInstance.occp_state |= umskb;   // mark as occupied
	env->Mtx->unlock(UART_StaticInstance.mtx); // exit critical section
	tch_uart_descriptor* uDesc = &UART_HWs[port];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_uart_bs* ubs = &UART_BD_CFGs[port];

                   // configure io port required by uart


	uDesc->_handle = uins = env->Mem->alloc(sizeof(tch_UartHandlePrototype));   // if successfully get io handle, create uart handle instance
	env->uStdLib->string->memset(uins,0,sizeof(tch_UartHandlePrototype));       // clear instance data structure

	uins->rxMtx = env->Mtx->create();
	uins->rxCondv = env->Condv->create();
	uins->txMtx = env->Mtx->create();
	uins->txCondv = env->Condv->create();
	uins->pno = port;
	uins->env = env;

	env->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_AF;
	iocfg.Af = ubs->afv;
	iocfg.popt = popt;

	io_msk = (1 << ubs->rxp) | (1 << ubs->txp);
	if(cfg->FlowCtrl && (ubs->rtsp != -1) && (ubs->ctsp != -1)){
		io_msk |= (1 << ubs->rtsp) | (1 << ubs->ctsp);
	}

	uins->ioHandle = env->Device->gpio->allocIo(env,ubs->port,io_msk,&iocfg,timeout); // try get io handle

	/*  Uart Baudrate Configuration  */
	uint8_t psc = 2;
	if(uDesc->_clkenr == &RCC->APB1ENR)
		psc = 4;

	uint32_t ioclk = SYS_CLK / psc;

	float udiv = (float)ioclk / (float)(cfg->Buadrate * 16);

	int mantisa = (int)udiv;
	if(mantisa > udiv)
		mantisa--;
	float frac = udiv - (float) mantisa;
	int dfrac = (int)(frac * 16);


	*uDesc->_clkenr |= uDesc->clkmsk; // enable clk source
	if(popt == ActOnSleep)  // if sleep - active should be supported, enable lp clk also
		*uDesc->_lpclkenr |= uDesc->lpclkmsk;


	*uDesc->_rstr |= uDesc->rstmsk;   // reset uart h/w
	*uDesc->_rstr &= ~uDesc->rstmsk;

	uhw->BRR = 0;   // initialize buadrate register
	uhw->BRR |= (mantisa << 4);
	uhw->BRR |= dfrac;  // set calcuated value

	uhw->CR1 = (cfg->Parity << USART_Parity_Pos_CR1) | USART_CR1_PEIE | USART_CR1_TE | USART_CR1_RE;   // enable interrupt packet error / transmit error / receiving error
	uhw->CR2 = (cfg->StopBit << USART_StopBit_Pos_CR2);  // set stop bit

	tch_DmaCfg dmaCfg;
	tch_lld_dma* DMA = (tch_lld_dma*)env->Device->dma;

	uins->txCh.txDma = NULL;
	uins->rxCh.rxDma = NULL;
	BOOL rxDma = FALSE;
	BOOL txDma = FALSE;

	if(ubs->txdma != DMA_NOT_USED){ // setup tx dma
		dmaCfg.BufferType = DMA_BufferMode_Normal;
		dmaCfg.Ch = ubs->txch;
		dmaCfg.Dir = DMA_Dir_MemToPeriph;
		dmaCfg.FlowCtrl = DMA_FlowControl_DMA;
		dmaCfg.Priority = DMA_Prior_Mid;
		dmaCfg.mAlign = DMA_DataAlign_Byte;
		dmaCfg.mBurstSize = DMA_Burst_Single;
		dmaCfg.mInc = TRUE;
		dmaCfg.pAlign = DMA_DataAlign_Byte;
		dmaCfg.pBurstSize = DMA_Burst_Single;
		dmaCfg.pInc = FALSE;

		uins->txCh.txDma = DMA->allocDma(env,ubs->txdma,&dmaCfg,timeout,popt); // can be null
		if(uins->txCh.txDma)
			txDma = TRUE;
	}else{
		uins->txCh.txQ = env->MsgQ->create(1);
	}


	if(ubs->rxdma != DMA_NOT_USED){ // setup rx dma
		dmaCfg.BufferType = DMA_BufferMode_Normal;
		dmaCfg.Ch = ubs->rxch;
		dmaCfg.Dir = DMA_Dir_PeriphToMem;
		dmaCfg.FlowCtrl = DMA_FlowControl_DMA;
		dmaCfg.Priority = DMA_Prior_Mid;
		dmaCfg.mAlign = DMA_DataAlign_Byte;
		dmaCfg.mBurstSize = DMA_Burst_Single;
		dmaCfg.mInc = TRUE;
		dmaCfg.pAlign = DMA_DataAlign_Byte;
		dmaCfg.pBurstSize = DMA_Burst_Single;
		dmaCfg.pInc = FALSE;

		uins->rxCh.rxDma = DMA->allocDma(env,ubs->rxdma,&dmaCfg,timeout,popt);  // can be null
		if(uins->rxCh.rxDma)
			rxDma = TRUE;
	}else{
		uins->rxCh.rxQ = env->MsgQ->create(TCH_URX_QSZ);
	}

	if(txDma){ // if tx dma is non-null (available), uart handle routines supporting dma are bound
		uins->pix.putc = tch_uartPutc;
		uins->pix.write = tch_uartWriteDma;

		uhw->CR1 &= ~USART_CR1_TCIE;
		uhw->CR3 |= USART_CR3_DMAT;
	}else{  // otherwise, non-dma routines are bound
		uins->pix.putc = tch_uartPutc;
		uins->pix.write = tch_uartWrite;

		uhw->CR3 &= ~USART_CR3_DMAT;
		uhw->CR1 |= USART_CR1_TCIE;
	}

	if(rxDma){
		uins->pix.getc = tch_uartGetc;
		uins->pix.read = tch_uartReadDma;

		uhw->CR1 &= ~USART_CR1_RXNEIE;
		uhw->CR3 |= USART_CR3_DMAR;
	}else{
		uins->pix.getc = tch_uartGetc;
		uins->pix.read = tch_uartRead;
		uhw->CR3 &= ~USART_CR3_DMAR;
	}

	uins->pix.close = tch_uartClose;


	uhw->CR1 |= USART_CR1_UE;
	uhw->SR = 0;


	tch_uartValidate(uins);
	NVIC_SetPriority(uDesc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(uDesc->irq);
	__DMB();
	__ISB();

	return (tch_UartHandle*) uins;
}


static tchStatus tch_uartClose(tch_UartHandle* handle){
	tch_UartHandlePrototype* ins = (tch_UartHandlePrototype*) handle;
	tch_uart_descriptor* uDesc;
	USART_TypeDef* uhw;
	tchStatus res = osOK;
	if(!handle)
		return osErrorParameter;
	if(!tch_uartIsValid(ins))
		return osErrorParameter;
	if(tch_port_isISR())
		return osErrorParameter;
	 uDesc = &UART_HWs[ins->pno];
	 uhw = (USART_TypeDef*) uDesc->_hw;
	const tch* env = ins->env;


	// block tx / rx operation by setting busy
	if(env->Mtx->lock(ins->txMtx,osWaitForever) != osOK)
		return FALSE;
	while(UART_IS_TXBUSY(ins)){
		env->Condv->wait(ins->txCondv,ins->txMtx,osWaitForever);
	}
	UART_SET_TXBUSY(ins);

	if(env->Mtx->lock(ins->rxMtx,osWaitForever) != osOK)
		return FALSE;
	while(UART_IS_RXBUSY(ins)){
		env->Condv->wait(ins->rxCondv,ins->rxMtx,osWaitForever);
	}
	UART_SET_RXBUSY(ins);

	tch_uartInvalidate(ins);       // invalidate instance
	env->Mtx->destroy(ins->rxMtx);
	env->Condv->destroy(ins->rxCondv);
	env->Mtx->destroy(ins->txMtx);
	env->Condv->destroy(ins->txCondv);

	if(ins->txCh.txDma){
		env->Device->dma->freeDma(ins->txCh.txDma);
		env->MsgQ->destroy(ins->txCh.txQ);
	}
	if(ins->rxCh.rxDma){
		env->Device->dma->freeDma(ins->rxCh.rxDma);
		env->MsgQ->destroy(ins->rxCh.rxQ);
	}
	ins->ioHandle->close(ins->ioHandle);
	if(env->Mtx->lock(UART_StaticInstance.mtx,osWaitForever) != osOK){
		return FALSE;
	}
	UART_StaticInstance.occp_state &= ~(1 << ins->pno); // clear Occupation flag
	UART_StaticInstance.lpoccp_state &= ~(1 << ins->pno);

	*uDesc->_rstr |= uDesc->rstmsk;
	*uDesc->_clkenr &= ~uDesc->clkmsk;
	*uDesc->_lpclkenr &= ~uDesc->lpclkmsk;
	NVIC_DisableIRQ(uDesc->irq);
	env->Condv->wakeAll(UART_StaticInstance.condv);
	env->Mem->free(handle);
	uDesc->_handle = NULL;
	return env->Mtx->unlock(UART_StaticInstance.mtx);
}


static tchStatus tch_uartPutc(tch_UartHandle* handle,uint8_t c){
	return handle->write(handle,&c,1);
}

static tchStatus tch_uartGetc(tch_UartHandle* handle,uint8_t* rc,uint32_t timeout){
	return handle->read(handle,rc,1,timeout);
}

static tchStatus tch_uartWrite(tch_UartHandle* handle,const uint8_t* bp,size_t sz){
	tch_UartHandlePrototype* ins;
	if(!handle)
		return osErrorParameter;
	 ins = (tch_UartHandlePrototype*) handle;
	if(!tch_uartIsValid(ins))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	size_t idx = 0;
	const tch* env = ins->env;
	osEvent evt;
	evt.status = osOK;


	if(env->Mtx->lock(ins->txMtx,osWaitForever) != osOK)
		return osErrorResource;
	while(UART_IS_TXBUSY(ins)){
		if((evt.status = env->Condv->wait(ins->txCondv,ins->txMtx,osWaitForever)) != osOK)
			return evt.status;
	}
	UART_SET_TXBUSY(ins);
	if((evt.status = env->Mtx->unlock(ins->txMtx)) != osOK)
		return evt.status;

	for(;idx < sz;idx++){
		while(!(uhw->SR & USART_SR_TXE)) /*NOP*/;
		uhw->DR = *((uint8_t*) bp + idx);
		evt = env->MsgQ->get(ins->txCh.txQ,osWaitForever);
		if(evt.status != osEventMessage)
			return evt.status;
	}

	if((evt.status = env->Mtx->lock(ins->txMtx,osWaitForever)) != osOK)
		return evt.status;

	UART_CLR_TXBUSY(ins);
	env->Condv->wakeAll(ins->txCondv);
	env->Mtx->unlock(ins->txMtx);
	return evt.status;
}



static tchStatus tch_uartRead(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout){
	tch_UartHandlePrototype* ins;
	if(!handle)
		return osErrorParameter;
	ins = (tch_UartHandlePrototype*) handle;
	if(!tch_uartIsValid(ins))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	const tch* env = ins->env;
	tchStatus result = osOK;
	*bp = '\0';
	size_t idx = 0;

	if((result = env->Mtx->lock(ins->rxMtx,timeout)) != osOK)
		return result;
	while(UART_IS_RXBUSY(ins)){
		if((result = env->Condv->wait(ins->rxCondv,ins->rxMtx,timeout)) != osOK)
			return result;
	}
	UART_SET_RXBUSY(ins);
	if((result = env->Mtx->unlock(ins->rxMtx)) != osOK)
		return result;

	if(uhw->SR & USART_SR_ORE)
		result = uhw->DR;
	uhw->CR1 |= USART_CR1_RXNEIE;       // enable usart rx inter
	osEvent evt;
	evt.status = osEventMessage;
	while(evt.status == osEventMessage){
		evt = env->MsgQ->get(ins->rxCh.rxQ,timeout);
		*(bp + idx++) = evt.value.v;
		if(idx >= sz)
			evt.status = osOK;
	}


	if((result = env->Mtx->lock(ins->rxMtx,timeout)) != osOK)
		return result;

	UART_CLR_RXBUSY(ins);
	env->Condv->wakeAll(ins->rxCondv);
	env->Mtx->unlock(ins->rxMtx);

	return evt.status;
}



static tchStatus tch_uartWriteDma(tch_UartHandle* handle,const uint8_t* bp,size_t sz){
	tch_UartHandlePrototype* ins = (tch_UartHandlePrototype*) handle;
	tchStatus result = osOK;
	if(!handle)
		return osErrorParameter;
	if(!tch_uartIsValid(ins))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	const tch* env = ins->env;


	if((result = env->Mtx->lock(ins->txMtx,osWaitForever)) != osOK)
		return result;

	while(UART_IS_TXBUSY(ins)){
		if((result = env->Condv->wait(ins->txCondv,ins->txMtx,osWaitForever)) != osOK)
			return result;
	}
	UART_SET_TXBUSY(ins);
	if((result = env->Mtx->unlock(ins->txMtx)) != osOK)
		return result;


	USART_TypeDef* uhw = (USART_TypeDef*)UART_HWs[ins->pno]._hw;
	tch_lld_dma* dma = (tch_lld_dma*) env->Device->dma;
	tch_DmaReqDef req;
	dma->initReq(&req,(uaddr_t) bp,(uaddr_t)&uhw->DR,sz);
	req.MemInc = TRUE;
	req.PeriphInc = FALSE;
	while(!(uhw->SR & USART_SR_TC)) __NOP();
	uhw->SR &= ~USART_SR_TC;    // clear tc
	while(!dma->beginXfer(ins->txCh.txDma,&req,osWaitForever,&result)){  // if dma fails, try again
		if(result == osErrorResource){
			return result;
		}
	}

	if((result = env->Mtx->lock(ins->txMtx,osWaitForever)) != osOK)
		return result;
	UART_CLR_TXBUSY(ins);
	env->Condv->wakeAll(ins->txCondv);
	env->Mtx->unlock(ins->txMtx);
	return osOK;
}

static tchStatus tch_uartReadDma(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout){
	tch_UartHandlePrototype* ins = (tch_UartHandlePrototype*) handle;
	if(!handle)
		return osErrorParameter;
	if(!tch_uartIsValid(ins))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	const tch* env = ins->env;
	tchStatus result = osOK;


	if((result = env->Mtx->lock(ins->rxMtx,timeout)) != osOK)
		return result;
	while(UART_IS_RXBUSY(ins)){
		if((result = env->Condv->wait(ins->rxCondv,ins->rxMtx,timeout)) != osOK)
			return result;
	}
	UART_SET_RXBUSY(ins);
	if((result = env->Mtx->unlock(ins->rxMtx)) != osOK)
		return result;

	USART_TypeDef* uhw = (USART_TypeDef*)UART_HWs[ins->pno]._hw;
	tch_lld_dma* dma = (tch_lld_dma*)env->Device->dma;
	tch_DmaReqDef req;
	dma->initReq(&req,(uaddr_t)bp,(uaddr_t) &uhw->DR,sz);
	req.MemInc = TRUE;
	req.PeriphInc = FALSE;
	while(!dma->beginXfer(ins->rxCh.rxDma,&req,timeout,&result)){
		if(result == osErrorResource)
			return result;
	}

	if((result = env->Mtx->lock(ins->rxMtx,timeout)) != osOK)
		return result;

	UART_CLR_RXBUSY(ins);
	env->Condv->wakeAll(ins->rxCondv);
	env->Mtx->unlock(ins->rxMtx);
	return osOK;
}


static inline void tch_uartValidate(tch_UartHandlePrototype* _handle){
	_handle->status = (((uint32_t) _handle) & 0xFFFF) ^ TCH_UART_CLASS_KEY;
}

static inline void tch_uartInvalidate(tch_UartHandlePrototype* _handle){
	_handle->status &= ~0xFFFF;

}

static inline BOOL tch_uartIsValid(tch_UartHandlePrototype* _handle){
	return (_handle->status & 0xFFFF) == ((((uint32_t) _handle) & 0xFFFF) ^ TCH_UART_CLASS_KEY);
}

static BOOL tch_uartInterruptHandler(tch_UartHandlePrototype* _handle,void* _hw){
	USART_TypeDef* uhw = (USART_TypeDef*) _hw;
	const tch* env = _handle->env;
	tchStatus result = osOK;
	uint16_t dm = 0;
	if(uhw->SR & USART_SR_RXNE){
		env->MsgQ->put(_handle->rxCh.rxQ,uhw->DR,0);
	}
	if(uhw->SR & USART_SR_TC){
		uhw->SR &= ~USART_SR_TC;
		env->MsgQ->put(_handle->txCh.txQ,osOK,0);
		return TRUE;
	}
	if(uhw->SR & USART_SR_ORE){
		dm = uhw->SR;
		dm = uhw->DR;
		return TRUE;
	}
	if(uhw->SR & USART_SR_FE){
		return FALSE;
	}
	if(uhw->SR & USART_SR_PE){
		return FALSE;
	}
	return FALSE;
}


void USART1_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[0];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_UartHandlePrototype* ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_uartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART2_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[1];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_UartHandlePrototype* ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_uartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART3_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[2];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_UartHandlePrototype* ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_uartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void UART4_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[3];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_UartHandlePrototype* ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_uartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}


