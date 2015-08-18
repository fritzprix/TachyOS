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
#include "tch_dma.h"
#include "tch_kernel.h"


#define TCH_UART_CLASS_KEY     ((uint16_t) 0x3D02)

#define TCH_URX_QSZ                         ((uint32_t) 10)

#define USART_Parity_Pos_CR1                (uint8_t) (9)

#define USART_StopBit_Pos_CR2               (uint8_t) (12)

#define SET_SAFE_EXIT();                    SAFE_EXIT:
#define RETURN_SAFELY()                     goto SAFE_EXIT;


#define UART_RXBUSY                         ((uint32_t) 0x10000)
#define UART_TXBUSY                         ((uint32_t) 0x20000)

#define UART_EVENT_TX_COMPLETE              ((uint32_t) 0x00000001)
#define UART_EVENT_RX_COMPLETE              ((uint32_t) 0x00000002)
#define UART_EVENT_OVR_ERROR                ((uint32_t) 0x00000004)
#define UART_EVENT_FRAME_ERROR              ((uint32_t) 0x00000008)
#define UART_EVENT_PARITY_ERROR             ((uint32_t) 0x00000010)

#define UART_EVENT_ERR_MSK                  (UART_EVENT_FRAME_ERROR |\
											 UART_EVENT_PARITY_ERROR|\
											 UART_EVENT_OVR_ERROR)

#define UART_EVENT_ALL                      (UART_EVENT_TX_COMPLETE|\
											 UART_EVENT_RX_COMPLETE|\
											 UART_EVENT_OVR_ERROR|\
											 UART_EVENT_FRAME_ERROR|\
											 UART_EVENT_PARITY_ERROR)



#define UART_SET_RXBUSY(ins)\
	do{\
		tch_kernelSetBusyMark();\
		((tch_usartHandlePrototype) ins)->status |= UART_RXBUSY;\
	}while(0)

#define UART_CLR_RXBUSY(ins)\
	do{\
		((tch_usartHandlePrototype) ins)->status &= ~UART_RXBUSY;\
		tch_kernelClrBusyMark();\
	}while(0)

#define UART_IS_RXBUSY(ins)         ((tch_usartHandlePrototype) ins)->status & UART_RXBUSY


#define UART_SET_TXBUSY(ins)\
	do{\
		tch_kernelSetBusyMark();\
		((tch_usartHandlePrototype) ins)->status |= UART_TXBUSY;\
	}while(0)

#define UART_CLR_TXBUSY(ins)\
	do{\
		((tch_usartHandlePrototype) ins)->status &= ~UART_TXBUSY;\
		tch_kernelClrBusyMark();\
	}while(0)

#define UART_IS_TXBUSY(ins)          ((tch_usartHandlePrototype) ins)->status & UART_TXBUSY


typedef struct tch_usart_request_s {
	uint8_t* bp;
	uint32_t sz;
}tch_usartRequest;

typedef struct tch_usart_handle_prototype_s {
	struct tch_usart_handle_s         pix;
	uart_t                            pno;
	tch_DmaHandle txDma;
	tch_DmaHandle rxDma;
	tch_eventId                       txEvId;
	tch_eventId                       rxEvId;
	tch_GpioHandle*                   ioHandle;
	tch_mtxId                         rxMtx;
	tch_condvId                       rxCondv;
	tch_mtxId                         txMtx;
	tch_condvId                       txCondv;
	uint32_t                          status;
	tch_usartRequest*                 txreq;
	tch_usartRequest*                 rxreq;
	const tch*                        env;
}* tch_usartHandlePrototype;

struct tch_usart_prototype_s {
	tch_lld_usart                     pix;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint16_t                          occp_state;
	uint16_t                          lpoccp_state;
};

static tch_usartHandle tch_usartOpen(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
static tchStatus tch_usartClose(tch_usartHandle handle);

static tchStatus tch_usartPutc(tch_usartHandle handle,uint8_t c);
static tchStatus tch_usartGetc(tch_usartHandle handle,uint8_t* rc,uint32_t timeout);
static tchStatus tch_usartWrite(tch_usartHandle handle,const uint8_t* bp,uint32_t sz);
static uint32_t tch_usartRead(tch_usartHandle handle,uint8_t* bp, uint32_t sz,uint32_t timeout);

static BOOL tch_usartInterruptHandler(tch_usartHandlePrototype _handle,void* _hw);
static inline void tch_usartValidate(tch_usartHandlePrototype _handle);
static inline void tch_usartInvalidate(tch_usartHandlePrototype _handle);
static inline BOOL tch_usartIsValid(tch_usartHandlePrototype _handle);

__attribute__((section(".data"))) static struct tch_usart_prototype_s UART_StaticInstance = {
		{
				MFEATURE_UART,
				tch_usartOpen
		},
		NULL,
		NULL,
		0,
		0
};

tch_lld_usart* tch_usartHalInit(const tch* env){
	if(UART_StaticInstance.mtx || UART_StaticInstance.condv)
		return NULL;
	if(!env)
		return NULL;
	UART_StaticInstance.occp_state = 0;
	UART_StaticInstance.lpoccp_state = 0;
	UART_StaticInstance.mtx = env->Mtx->create();
	UART_StaticInstance.condv = env->Condv->create();

	return (tch_lld_usart*) &UART_StaticInstance;
}


static tch_usartHandle tch_usartOpen(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	tch_usartHandlePrototype uins = NULL;
	tch_GpioCfg iocfg;
	uint32_t io_msk = 0;
	uint32_t umskb = 1 << port;

	if(port >= MFEATURE_UART)    // if requested channel is larger than uart channel count
		return NULL;                    // return null
	if(tch_port_isISR())
		return NULL;

	if(env->Mtx->lock(UART_StaticInstance.mtx,timeout) != tchOK)
		return NULL;
	while(UART_StaticInstance.occp_state & umskb){
		if(env->Condv->wait(UART_StaticInstance.condv,UART_StaticInstance.mtx,timeout) != tchOK)
			return NULL;
	}
	UART_StaticInstance.occp_state |= umskb;   // mark as occupied
	env->Mtx->unlock(UART_StaticInstance.mtx); // exit critical section
	tch_uart_descriptor* uDesc = &UART_HWs[port];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_uart_bs* ubs = &UART_BD_CFGs[port];

                   // configure io port required by uart


	uDesc->_handle = uins = env->Mem->alloc(sizeof(struct tch_usart_handle_prototype_s));   // if successfully get io handle, create uart handle instance
	env->uStdLib->string->memset(uins,0,sizeof(struct tch_usart_handle_prototype_s));       // clear instance data structure

	uins->rxMtx = env->Mtx->create();
	uins->rxCondv = env->Condv->create();
	uins->txMtx = env->Mtx->create();
	uins->txCondv = env->Condv->create();
	uins->txEvId = env->Event->create();
	uins->rxEvId = env->Event->create();
	uins->txreq = NULL;
	uins->rxreq = NULL;
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

	uins->txDma = NULL;
	uins->rxDma = NULL;

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

		uins->txDma = tch_dma->allocate(env,ubs->txdma,&dmaCfg,timeout,popt); // can be null
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

		uins->rxDma = tch_dma->allocate(env,ubs->rxdma,&dmaCfg,timeout,popt);  // can be null
	}


	uins->pix.put = tch_usartPutc;
	uins->pix.get = tch_usartGetc;
	uins->pix.write = tch_usartWrite;
	uins->pix.read = tch_usartRead;
	uins->pix.close = tch_usartClose;


	uhw->CR1 |= USART_CR1_UE;
	uhw->SR = 0;


	tch_usartValidate(uins);
	/*
	NVIC_SetPriority(uDesc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(uDesc->irq);
	*/
	tch_kernel_enableInterrupt(uDesc->irq,HANDLER_NORMAL_PRIOR);

	return (tch_usartHandle) uins;
}


static tchStatus tch_usartClose(tch_usartHandle handle){
	tch_usartHandlePrototype ins = (tch_usartHandlePrototype) handle;
	tch_uart_descriptor* uDesc;
	USART_TypeDef* uhw;
	tchStatus res = tchOK;
	if(!handle)
		return tchErrorParameter;
	if(!tch_usartIsValid(ins))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorParameter;
	 uDesc = &UART_HWs[ins->pno];
	 uhw = (USART_TypeDef*) uDesc->_hw;
	const tch* env = ins->env;


	// block tx / rx operation by setting busy
	if(env->Mtx->lock(ins->txMtx,tchWaitForever) != tchOK)
		return FALSE;
	while(UART_IS_TXBUSY(ins)){
		env->Condv->wait(ins->txCondv,ins->txMtx,tchWaitForever);
	}
	UART_SET_TXBUSY(ins);

	if(env->Mtx->lock(ins->rxMtx,tchWaitForever) != tchOK)
		return FALSE;
	while(UART_IS_RXBUSY(ins)){
		env->Condv->wait(ins->rxCondv,ins->rxMtx,tchWaitForever);
	}
	UART_SET_RXBUSY(ins);

	tch_usartInvalidate(ins);       // invalidate instance
	env->Mtx->destroy(ins->rxMtx);
	env->Condv->destroy(ins->rxCondv);
	env->Mtx->destroy(ins->txMtx);
	env->Condv->destroy(ins->txCondv);
	env->Event->destroy(ins->rxEvId);
	env->Event->destroy(ins->txEvId);

	if(ins->txDma){
		tch_dma->freeDma(ins->txDma);
	}
	if(ins->rxDma){
		tch_dma->freeDma(ins->rxDma);
	}
	ins->ioHandle->close(ins->ioHandle);
	if(env->Mtx->lock(UART_StaticInstance.mtx,tchWaitForever) != tchOK){
		return FALSE;
	}
	UART_StaticInstance.occp_state &= ~(1 << ins->pno); // clear Occupation flag
	UART_StaticInstance.lpoccp_state &= ~(1 << ins->pno);

	*uDesc->_rstr |= uDesc->rstmsk;
	*uDesc->_clkenr &= ~uDesc->clkmsk;
	*uDesc->_lpclkenr &= ~uDesc->lpclkmsk;
//	NVIC_DisableIRQ(uDesc->irq);
	tch_kernel_disableInterrupt(uDesc->irq);
	env->Condv->wakeAll(UART_StaticInstance.condv);
	UART_CLR_RXBUSY(ins);
	UART_CLR_TXBUSY(ins);
	env->Mem->free(handle);
	uDesc->_handle = NULL;
	return env->Mtx->unlock(UART_StaticInstance.mtx);
}


static tchStatus tch_usartPutc(tch_usartHandle handle,uint8_t c){
	return handle->write(handle,&c,1);
}

static tchStatus tch_usartGetc(tch_usartHandle handle,uint8_t* rc,uint32_t timeout){
	return handle->read(handle,rc,1,timeout) == 1? tchOK : tchErrorIo;
}

static tchStatus tch_usartWrite(tch_usartHandle handle,const uint8_t* bp,uint32_t sz){
	tch_usartHandlePrototype ins;
	tchStatus result;
	uint32_t ev;
	if(!handle)
		return tchErrorParameter;
	 ins = (tch_usartHandlePrototype) handle;
	if(!tch_usartIsValid(ins))
		return tchErrorResource;
	if(tch_port_isISR())
		return tchErrorISR;

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	uint32_t idx = 0;
	const tch* env = ins->env;
	result = tchOK;

	if(env->Mtx->lock(ins->txMtx,tchWaitForever) != tchOK)
		return tchErrorResource;
	while(UART_IS_TXBUSY(ins)){
		if((result = env->Condv->wait(ins->txCondv,ins->txMtx,tchWaitForever)) != tchOK)
			return result;
	}
	UART_SET_TXBUSY(ins);
	if((result = env->Mtx->unlock(ins->txMtx)) != tchOK)
		return result;

	if(!ins->txDma){
		tch_usartRequest tx_req;

		ins->txreq = &tx_req;
		tx_req.sz = sz;
		tx_req.bp = (uint8_t*) bp;
		tx_req.sz--;
		uhw->CR1 |= USART_CR1_TCIE;
		uhw->DR = *(tx_req.bp++);

		if((result = ins->env->Event->wait(ins->txEvId,UART_EVENT_TX_COMPLETE,tchWaitForever)) != tchOK){
			ev = ins->env->Event->clear(ins->txEvId,UART_EVENT_TX_COMPLETE);
			RETURN_SAFELY();
		}

		ev = ins->env->Event->clear(ins->txEvId,UART_EVENT_TX_COMPLETE);
		if(ev & UART_EVENT_ERR_MSK){
			ins->env->Event->clear(ins->txEvId,UART_EVENT_ERR_MSK);
			result = tchErrorIo;
			RETURN_SAFELY();
		}
	} else {
		uhw->CR3 |= USART_CR3_DMAT;
		tch_DmaReqDef req;
		tch_dma->initReq(&req,(uaddr_t) bp,(uaddr_t)&uhw->DR,sz);
		req.MemInc = TRUE;
		req.PeriphInc = FALSE;
		while(!(uhw->SR & USART_SR_TC)) __NOP();
		uhw->SR &= ~USART_SR_TC;    // clear tc
		if(tch_dma->beginXfer(ins->txDma,&req,tchWaitForever,&result)){  // if dma fails, try again
			result = tchErrorIo;
			RETURN_SAFELY();
		}
	}

	result = tchOK;
	SET_SAFE_EXIT();

	env->Mtx->lock(ins->txMtx,tchWaitForever);
	ins->txreq = NULL;
	uhw->CR1 &= ~USART_CR1_TCIE;
	uhw->CR3 &= ~USART_CR3_DMAT;
	UART_CLR_TXBUSY(ins);
	env->Condv->wakeAll(ins->txCondv);
	env->Mtx->unlock(ins->txMtx);
	return result;
}



static uint32_t tch_usartRead(tch_usartHandle handle,uint8_t* bp, uint32_t sz,uint32_t timeout){
	tch_usartHandlePrototype ins;
	uint32_t ev;
	if(!handle)
		return 0;
	ins = (tch_usartHandlePrototype) handle;
	if(!tch_usartIsValid(ins))
		return tchErrorResource;
	if(tch_port_isISR())
		return 0;

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	const tch* env = ins->env;

	*bp = '\0';
	uint32_t idx = 0;

	if(env->Mtx->lock(ins->rxMtx,timeout) != tchOK)
		return 0;
	while(UART_IS_RXBUSY(ins)){
		if(env->Condv->wait(ins->rxCondv,ins->rxMtx,timeout) != tchOK)
			return 0;
	}
	UART_SET_RXBUSY(ins);
	if(env->Mtx->unlock(ins->rxMtx) != tchOK)
		return 0;

	if(!ins->rxDma){
		tch_usartRequest rx_req;
		rx_req.bp = bp;
		rx_req.sz = sz;
		ins->rxreq = &rx_req;

		uhw->CR1 |= USART_CR1_RXNEIE;       // enable usart rx inter
		env->Event->wait(ins->rxEvId,UART_EVENT_RX_COMPLETE,timeout);
		ev = env->Event->clear(ins->rxEvId,UART_EVENT_ALL);
		sz -= rx_req.sz;
	} else {
		uhw->CR3 |= USART_CR3_DMAR;
		tch_DmaReqDef req;
		tch_dma->initReq(&req,(uaddr_t)bp,(uaddr_t) &uhw->DR,sz);
		req.MemInc = TRUE;
		req.PeriphInc = FALSE;
		sz -= tch_dma->beginXfer(ins->rxDma,&req,timeout,NULL);
	}

	env->Mtx->lock(ins->rxMtx,timeout);
	uhw->CR1 &= ~USART_CR1_RXNEIE;
	uhw->CR3 &= ~USART_CR3_DMAR;
	UART_CLR_RXBUSY(ins);
	env->Condv->wakeAll(ins->rxCondv);
	env->Mtx->unlock(ins->rxMtx);

	return sz;
}


static inline void tch_usartValidate(tch_usartHandlePrototype _handle){
	_handle->status = (((uint32_t) _handle) & 0xFFFF) ^ TCH_UART_CLASS_KEY;
}

static inline void tch_usartInvalidate(tch_usartHandlePrototype _handle){
	_handle->status &= ~0xFFFF;

}

static inline BOOL tch_usartIsValid(tch_usartHandlePrototype _handle){
	return (_handle->status & 0xFFFF) == ((((uint32_t) _handle) & 0xFFFF) ^ TCH_UART_CLASS_KEY);
}

static BOOL tch_usartInterruptHandler(tch_usartHandlePrototype ins,void* _hw){
	USART_TypeDef* uhw = (USART_TypeDef*) _hw;
	const tch* env = ins->env;
	uint16_t dm = 0;
	if(uhw->SR & USART_SR_RXNE){
		if(ins->rxreq){
			if(ins->rxreq->sz--){
				*(ins->rxreq->bp++) = uhw->DR;
				if(!ins->rxreq->sz){
					uhw->CR1 &= ~USART_CR1_RXNEIE;
					env->Event->set(ins->rxEvId,UART_EVENT_RX_COMPLETE);
					return TRUE;
				}
			}else{
				dm = uhw->DR; // read out unexpectedly recieved character
			}
		}
	}
	if(uhw->SR & USART_SR_TC){
		if(ins->txreq){
			if(ins->txreq->sz--){
				uhw->DR = *(ins->txreq->bp++);
			}else{
				env->Event->set(ins->txEvId,UART_EVENT_TX_COMPLETE);
				uhw->SR &= ~USART_SR_TC;
				return TRUE;
			}
		}
	}
	if(uhw->SR & USART_SR_ORE){
		if(ins->rxreq){
			if(ins->rxreq->sz--){
				*(ins->rxreq->bp++) = uhw->DR;
				if(!ins->rxreq->sz){
					uhw->CR1 &= ~USART_CR1_RXNEIE;
					env->Event->set(ins->rxEvId,UART_EVENT_RX_COMPLETE);
					return TRUE;
				}
				return TRUE;
			}
		}
		dm = uhw->SR;
		dm = uhw->DR;
	}
	if(uhw->SR & USART_SR_FE){
		if(ins->rxreq)
			env->Event->set(ins->rxEvId,UART_EVENT_FRAME_ERROR);
		if(ins->txreq)
			env->Event->set(ins->txEvId,UART_EVENT_FRAME_ERROR);
		return FALSE;
	}
	if(uhw->SR & USART_SR_PE){
		if(ins->rxreq)
			env->Event->set(ins->rxEvId,UART_EVENT_PARITY_ERROR);
		if(ins->txreq)
			env->Event->set(ins->txEvId,UART_EVENT_PARITY_ERROR);
		return FALSE;
	}
	return FALSE;
}


void USART1_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[0];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART2_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[1];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART3_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[2];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void UART4_IRQHandler(void){
	tch_uart_descriptor* uDesc = &UART_HWs[3];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins){ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usartInterruptHandler(ins,uhw)){
		uhw->SR &= ~uhw->SR;  // clear
	}
}


