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
#include "tch_gpio.h"
#include "tch_usart.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/util/string.h"



#ifndef UART_CLASS_KEY
#define UART_CLASS_KEY						((uint16_t) 0x3D02)
#endif


#define TCH_URX_QSZ							((uint32_t) 10)

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
		set_system_busy();\
		((tch_usartHandlePrototype) ins)->status |= UART_RXBUSY;\
	}while(0)

#define UART_CLR_RXBUSY(ins)\
	do{\
		((tch_usartHandlePrototype) ins)->status &= ~UART_RXBUSY;\
		clear_system_busy();\
	}while(0)

#define UART_IS_RXBUSY(ins)         ((tch_usartHandlePrototype) ins)->status & UART_RXBUSY

#define UART_SET_TXBUSY(ins)\
	do{\
		set_system_busy();\
		((tch_usartHandlePrototype) ins)->status |= UART_TXBUSY;\
	}while(0)

#define UART_CLR_TXBUSY(ins)\
	do{\
		((tch_usartHandlePrototype) ins)->status &= ~UART_TXBUSY;\
		clear_system_busy();\
	}while(0)

#define UART_IS_TXBUSY(ins)          ((tch_usartHandlePrototype) ins)->status & UART_TXBUSY


typedef struct tch_usart_request_s{
	uint8_t* bp;
	uint32_t sz;
}tch_usartRequest;

typedef struct tch_usart_handle_prototype_s {
	struct tch_usart_handle_s        pix;
	uart_t               	         pno;
	tch_dmaHandle					 txDma;
	tch_dmaHandle					 rxDma;
	tch_eventId                      txEvId;
	tch_eventId                      rxEvId;
	tch_gpioHandle*                  ioHandle;
	tch_mtxId                        rxMtx;
	tch_condvId                      rxCondv;
	tch_mtxId                        txMtx;
	tch_condvId                      txCondv;
	uint32_t                         status;
	tch_usartRequest*                txreq;
	tch_usartRequest*                rxreq;
	const tch*                       env;
}* tch_usartHandlePrototype;


__USER_API__ static tch_usartHandle tch_usart_open(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
__USER_API__ static tchStatus tch_usart_close(tch_usartHandle handle);
__USER_API__ static tchStatus tch_usart_write(tch_usartHandle handle,const uint8_t* bp,uint32_t sz);
__USER_API__ static uint32_t tch_usart_read(tch_usartHandle handle,uint8_t* bp, uint32_t sz,uint32_t timeout);


static int tch_usart_init(void);
static void tch_usart_exit(void);
static BOOL tch_usart_interruptHandler(tch_usartHandlePrototype _handle,void* _hw);
static inline void tch_usart_validate(tch_usartHandlePrototype _handle);
static inline void tch_usart_invalidate(tch_usartHandlePrototype _handle);
static inline BOOL tch_usart_isValid(tch_usartHandlePrototype _handle);


__USER_RODATA__ tch_device_service_usart UART_Ops = {
		.count = MFEATURE_UART,
		.allocate = tch_usart_open
};

static tch_mtxCb 		mtx;
static tch_condvCb 		condv;
static uint16_t			occp_state;
static uint16_t			lpoccp_state;
static tch_device_service_dma*		dma;


static int tch_usart_init(void)
{
	dma = NULL;
	occp_state = 0;
	lpoccp_state = 0;
	tch_mutexInit(&mtx);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_UART,UART_CLASS_KEY,&UART_Ops,FALSE);
}

static void tch_usart_exit(void)
{
	tch_kmod_unregister(MODULE_TYPE_UART,UART_CLASS_KEY);
	tch_mutexDeinit(&mtx);
	tch_condvDeinit(&condv);
}

MODULE_INIT(tch_usart_init);
MODULE_EXIT(tch_usart_exit);

static tch_usartHandle tch_usart_open(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt)
{
	tch_usartHandlePrototype uins = NULL;
	gpio_config_t iocfg;
	uint32_t io_msk = 0;
	uint32_t umskb = 1 << port;


	if(!dma)
	{
		dma = tch_kmod_request(MODULE_TYPE_DMA);
	}

	if(port >= MFEATURE_UART)
	{// if requested channel is larger than uart channel count
		return NULL;                    // return null
	}
	if(tch_port_isISR())
	{
		return NULL;
	}

	tch_device_service_gpio* gpio = (tch_device_service_gpio*) Service->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		return NULL;
	}

	if(env->Mtx->lock(&mtx,timeout) != tchOK)
	{
		return NULL;
	}
	while(occp_state & umskb)
	{
		if(env->Condv->wait(&condv,&mtx,timeout) != tchOK)
		{
			return NULL;
		}
	}
	occp_state |= umskb;   // mark as occupied
	env->Mtx->unlock(&mtx); // exit critical section
	tch_uart_descriptor* uDesc = &UART_HWs[port];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_uart_bs_t* ubs = &UART_BD_CFGs[port];

	uDesc->_handle = uins = env->Mem->alloc(sizeof(struct tch_usart_handle_prototype_s));   // if successfully get io handle, create uart handle instance
	mset(uins,0,sizeof(struct tch_usart_handle_prototype_s));       // clear instance data structure

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


	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_AF;
	iocfg.Af = ubs->afv;
	iocfg.popt = popt;

	io_msk = (1 << ubs->rxp) | (1 << ubs->txp);
	if(cfg->FlowCtrl && (ubs->rtsp != -1) && (ubs->ctsp != -1))
	{
		io_msk |= (1 << ubs->rtsp) | (1 << ubs->ctsp);
	}

	uins->ioHandle = gpio->allocIo(env,ubs->port,io_msk,&iocfg,timeout); // try get io handle

	/*  Uart Baudrate Configuration  */
	uint8_t psc = 2;
	if(uDesc->_clkenr == &RCC->APB1ENR)
	{
		psc = 4;
	}

	uint32_t ioclk = SYS_CLK / psc;
	float udiv = (float)ioclk / (float)(cfg->Buadrate * 16);
	int mantisa = (int)udiv;
	if(mantisa > udiv)
	{
		mantisa--;
	}
	float frac = udiv - (float) mantisa;
	int dfrac = (int)(frac * 16);


	*uDesc->_clkenr |= uDesc->clkmsk; // enable clk source
	if(popt == ActOnSleep)
	{// if sleep - active should be supported, enable lp clk also
		*uDesc->_lpclkenr |= uDesc->lpclkmsk;
	}

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

	if((ubs->txdma != DMA_NOT_USED) && dma)
	{ // setup tx dma
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

		uins->txDma = dma->allocate(env,ubs->txdma,&dmaCfg,timeout,popt); // can be null
	}

	if((ubs->rxdma != DMA_NOT_USED) && dma)
	{ // setup rx dma
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

		uins->rxDma = dma->allocate(env,ubs->rxdma,&dmaCfg,timeout,popt);  // can be null
	}

	uins->pix.write = tch_usart_write;
	uins->pix.read = tch_usart_read;
	uins->pix.close = tch_usart_close;

	uhw->CR1 |= USART_CR1_UE;
	uhw->SR = 0;

	tch_usart_validate(uins);
	tch_enableInterrupt(uDesc->irq,HANDLER_NORMAL_PRIOR);

	return (tch_usartHandle) uins;
}


static tchStatus tch_usart_close(tch_usartHandle handle)
{
	tch_usartHandlePrototype ins = (tch_usartHandlePrototype) handle;
	tch_uart_descriptor* uDesc;
	USART_TypeDef* uhw;
	tchStatus res = tchOK;
	if(!handle)
	{
		return tchErrorParameter;
	}
	if(!tch_usart_isValid(ins))
	{
		return tchErrorParameter;
	}
	if(tch_port_isISR())
	{
		return tchErrorParameter;
	}
	uDesc = &UART_HWs[ins->pno];
	uhw = (USART_TypeDef*) uDesc->_hw;
	const tch* env = ins->env;

	// block tx / rx operation by setting busy
	if(env->Mtx->lock(ins->txMtx,tchWaitForever) != tchOK)
	{
		return FALSE;
	}
	while(UART_IS_TXBUSY(ins))
	{
		env->Condv->wait(ins->txCondv,ins->txMtx,tchWaitForever);
	}
	UART_SET_TXBUSY(ins);

	if(env->Mtx->lock(ins->rxMtx,tchWaitForever) != tchOK)
	{
		return FALSE;
	}
	while(UART_IS_RXBUSY(ins))
	{
		env->Condv->wait(ins->rxCondv,ins->rxMtx,tchWaitForever);
	}
	UART_SET_RXBUSY(ins);

	tch_usart_invalidate(ins);       // invalidate instance
	env->Mtx->destroy(ins->rxMtx);
	env->Condv->destroy(ins->rxCondv);
	env->Mtx->destroy(ins->txMtx);
	env->Condv->destroy(ins->txCondv);
	env->Event->destroy(ins->rxEvId);
	env->Event->destroy(ins->txEvId);

	if(ins->txDma && dma)
	{
		dma->freeDma(ins->txDma);
	}
	if(ins->rxDma && dma)
	{
		dma->freeDma(ins->rxDma);
	}
	ins->ioHandle->close(ins->ioHandle);
	if(env->Mtx->lock(&mtx,tchWaitForever) != tchOK)
	{
		return FALSE;
	}
	occp_state &= ~(1 << ins->pno); // clear Occupation flag
	lpoccp_state &= ~(1 << ins->pno);

	*uDesc->_rstr |= uDesc->rstmsk;
	*uDesc->_clkenr &= ~uDesc->clkmsk;
	*uDesc->_lpclkenr &= ~uDesc->lpclkmsk;
	tch_disableInterrupt(uDesc->irq);
	env->Condv->wakeAll(&condv);
	UART_CLR_RXBUSY(ins);
	UART_CLR_TXBUSY(ins);
	env->Mem->free(handle);
	uDesc->_handle = NULL;
	return env->Mtx->unlock(&mtx);
}


static tchStatus tch_usart_write(tch_usartHandle handle,const uint8_t* bp,uint32_t sz)
{
	tch_usartHandlePrototype ins;
	tchStatus result;
	uint32_t ev;
	if(!handle)
	{
		return tchErrorParameter;
	}
	ins = (tch_usartHandlePrototype) handle;
	if(!tch_usart_isValid(ins))
	{
		return tchErrorResource;
	}
	if(tch_port_isISR())
	{
		return tchErrorISR;
	}

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	uint32_t idx = 0;
	const tch* env = ins->env;
	result = tchOK;

	if(env->Mtx->lock(ins->txMtx,tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}
	while(UART_IS_TXBUSY(ins))
	{
		if((result = env->Condv->wait(ins->txCondv,ins->txMtx,tchWaitForever)) != tchOK)
			return result;
	}
	UART_SET_TXBUSY(ins);
	if((result = env->Mtx->unlock(ins->txMtx)) != tchOK)
	{
		return result;
	}

	if(!ins->txDma || !dma)
	{
		tch_usartRequest tx_req;

		ins->txreq = &tx_req;
		tx_req.sz = sz;
		tx_req.bp = (uint8_t*) bp;
		tx_req.sz--;
		uhw->CR1 |= USART_CR1_TCIE;
		uhw->DR = *(tx_req.bp++);

		if((result = ins->env->Event->wait(ins->txEvId,UART_EVENT_TX_COMPLETE,tchWaitForever)) != tchOK)
		{
			ev = ins->env->Event->clear(ins->txEvId,UART_EVENT_TX_COMPLETE);
			RETURN_SAFELY();
		}

		ev = ins->env->Event->clear(ins->txEvId,UART_EVENT_TX_COMPLETE);
		if(ev & UART_EVENT_ERR_MSK)
		{
			ins->env->Event->clear(ins->txEvId,UART_EVENT_ERR_MSK);
			result = tchErrorIo;
			RETURN_SAFELY();
		}
	}
	else
	{
		uhw->CR3 |= USART_CR3_DMAT;
		tch_DmaReqDef req;
		dma->initReq(&req,(uaddr_t) bp,(uaddr_t)&uhw->DR,sz);
		req.MemInc = TRUE;
		req.PeriphInc = FALSE;
		while(!(uhw->SR & USART_SR_TC)) __NOP();
		uhw->SR &= ~USART_SR_TC;    // clear tc
		if(dma->beginXfer(ins->txDma,&req,tchWaitForever,&result)){  // if dma fails, try again
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



static uint32_t tch_usart_read(tch_usartHandle handle,uint8_t* bp, uint32_t sz,uint32_t timeout)
{
	tch_usartHandlePrototype ins;
	uint32_t ev;
	if(!handle)
	{
		return 0;
	}
	ins = (tch_usartHandlePrototype) handle;
	if(!tch_usart_isValid(ins))
	{
		return tchErrorResource;
	}
	if(tch_port_isISR())
	{
		return 0;
	}

	USART_TypeDef* uhw = UART_HWs[ins->pno]._hw;
	const tch* env = ins->env;

	*bp = '\0';
	uint32_t idx = 0;

	if(env->Mtx->lock(ins->rxMtx,timeout) != tchOK)
	{
		return 0;
	}
	while(UART_IS_RXBUSY(ins))
	{
		if(env->Condv->wait(ins->rxCondv,ins->rxMtx,timeout) != tchOK)
		{
			return 0;
		}
	}
	UART_SET_RXBUSY(ins);
	if(env->Mtx->unlock(ins->rxMtx) != tchOK)
	{
		return 0;
	}

	if(!ins->rxDma && !dma)
	{
		tch_usartRequest rx_req;
		rx_req.bp = bp;
		rx_req.sz = sz;
		ins->rxreq = &rx_req;

		uhw->CR1 |= USART_CR1_RXNEIE;       // enable usart rx inter
		env->Event->wait(ins->rxEvId,UART_EVENT_RX_COMPLETE,timeout);
		ev = env->Event->clear(ins->rxEvId,UART_EVENT_ALL);
		sz -= rx_req.sz;
	}
	else
	{
		uhw->CR3 |= USART_CR3_DMAR;
		tch_DmaReqDef req;
		dma->initReq(&req,(uaddr_t)bp,(uaddr_t) &uhw->DR,sz);
		req.MemInc = TRUE;
		req.PeriphInc = FALSE;
		sz -= dma->beginXfer(ins->rxDma,&req,timeout,NULL);
	}

	env->Mtx->lock(ins->rxMtx,timeout);
	uhw->CR1 &= ~USART_CR1_RXNEIE;
	uhw->CR3 &= ~USART_CR3_DMAR;
	UART_CLR_RXBUSY(ins);
	env->Condv->wakeAll(ins->rxCondv);
	env->Mtx->unlock(ins->rxMtx);
	return sz;
}


static inline void tch_usart_validate(tch_usartHandlePrototype _handle)
{
	_handle->status = (((uint32_t) _handle) & 0xFFFF) ^ UART_CLASS_KEY;
}

static inline void tch_usart_invalidate(tch_usartHandlePrototype _handle)
{
	_handle->status &= ~0xFFFF;

}

static inline BOOL tch_usart_isValid(tch_usartHandlePrototype _handle)
{
	return (_handle->status & 0xFFFF) == ((((uint32_t) _handle) & 0xFFFF) ^ UART_CLASS_KEY);
}

static BOOL tch_usart_interruptHandler(tch_usartHandlePrototype ins,void* _hw)
{
	USART_TypeDef* uhw = (USART_TypeDef*) _hw;
	const tch* env = ins->env;
	uint16_t dm = 0;
	if(uhw->SR & USART_SR_RXNE)
	{
		if(ins->rxreq){
			if(ins->rxreq->sz--)
			{
				*(ins->rxreq->bp++) = uhw->DR;
				if(!ins->rxreq->sz)
				{
					uhw->CR1 &= ~USART_CR1_RXNEIE;
					env->Event->set(ins->rxEvId,UART_EVENT_RX_COMPLETE);
					return TRUE;
				}
			}else
			{
				dm = uhw->DR; // read out unexpectedly recieved character
			}
		}
	}
	if(uhw->SR & USART_SR_TC)
	{
		if(ins->txreq)
		{
			if(ins->txreq->sz--)
			{
				uhw->DR = *(ins->txreq->bp++);
			}
			else
			{
				env->Event->set(ins->txEvId,UART_EVENT_TX_COMPLETE);
				uhw->SR &= ~USART_SR_TC;
				return TRUE;
			}
		}
	}
	if(uhw->SR & USART_SR_ORE)
	{
		if(ins->rxreq)
		{
			if(ins->rxreq->sz--)
			{
				*(ins->rxreq->bp++) = uhw->DR;
				if(!ins->rxreq->sz)
				{
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
	if(uhw->SR & USART_SR_FE)
	{
		if(ins->rxreq)
		{
			env->Event->set(ins->rxEvId,UART_EVENT_FRAME_ERROR);
		}
		if(ins->txreq)
		{
			env->Event->set(ins->txEvId,UART_EVENT_FRAME_ERROR);
		}
		return FALSE;
	}
	if(uhw->SR & USART_SR_PE)
	{
		if(ins->rxreq)
		{
			env->Event->set(ins->rxEvId,UART_EVENT_PARITY_ERROR);
		}

		if(ins->txreq)
		{
			env->Event->set(ins->txEvId,UART_EVENT_PARITY_ERROR);
		}
		return FALSE;
	}
	return FALSE;
}


void USART1_IRQHandler(void)
{
	tch_uart_descriptor* uDesc = &UART_HWs[0];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins)
	{ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usart_interruptHandler(ins,uhw))
	{
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART2_IRQHandler(void)
{
	tch_uart_descriptor* uDesc = &UART_HWs[1];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins)
	{ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usart_interruptHandler(ins,uhw))
	{
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void USART3_IRQHandler(void)
{
	tch_uart_descriptor* uDesc = &UART_HWs[2];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins)
	{ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usart_interruptHandler(ins,uhw))
	{
		uhw->SR &= ~uhw->SR;  // clear
	}
}

void UART4_IRQHandler(void)
{
	tch_uart_descriptor* uDesc = &UART_HWs[3];
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	tch_usartHandlePrototype ins = uDesc->_handle;
	if(!ins)
	{ // if handle is not bound to io, clear raised interrupt
		uint8_t dummy = uhw->DR;
		uhw->SR = 0;
		return;
	}
	if(!tch_usart_interruptHandler(ins,uhw))
	{
		uhw->SR &= ~uhw->SR;  // clear
	}
}


