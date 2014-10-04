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
#include "tch_halcfg.h"


#define TCH_UART_CLASS_KEY     ((uint16_t) 0x3D02)





#define USART_Parity_Pos_CR1                (uint8_t) (9)
#define USART_Parity_NON                    (uint8_t) (0 << 1 | 0 << 0)
#define USART_Parity_ODD                    (uint8_t) (1 << 1 | 1 << 0)
#define USART_Parity_EVEN                   (uint8_t) (1 << 1 | 0 << 0)

#define USART_StopBit_Pos_CR2               (uint8_t) (12)
#define USART_StopBit_1B                    (uint8_t) (0)
#define USART_StopBit_0dot5B                (uint8_t) (1)
#define USART_StopBit_2B                    (uint8_t) (2)
#define USART_StopBit_1dot5B                (uint8_t) (3)


#define INIT_UART_STOPBIT                   {\
	                                          USART_StopBit_0dot5B,\
	                                          USART_StopBit_1B,\
	                                          USART_StopBit_1dot5B,\
	                                          USART_StopBit_2B\
}


#define INIT_UART_PARITY                   {\
	                                          USART_Parity_NON,\
	                                          USART_Parity_ODD,\
	                                          USART_Parity_EVEN\
}







typedef struct tch_lld_usart_handle_prototype_t {
	tch_UartHandle                    pix;
	tch_DmaHandle*                    txDma;
	tch_DmaHandle*                    rxDma;
	tch_GpioHandle*                   ioHandle;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint32_t                          status;
	tch*                              env;
}tch_UartHandlePrototype;

typedef struct tch_lld_uart_prototype {
	tch_lld_usart                     pix;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint16_t                          occp_state;
	uint16_t                          lpoccp_state;
}tch_lld_uart_prototype;

static tch_UartHandle* tch_uartOpen(tch* env,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
static void* tch_uartClose(tch_UartHandle* handle);

static tchStatus tch_uartPutc(tch_UartHandle* handle,uint8_t c);
static uint8_t tch_uartGetc(tch_UartHandle* handle,tchStatus* result);
static tchStatus tch_uartWrite(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
static tchStatus tch_uartRead(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);
static size_t tch_uartAvailable(tch_UartHandle* handle);
static tchStatus tch_uartWriteCstr(tch_UartHandle* handle,const char* cstr);
static tchStatus tch_uartReadCstr(tch_UartHandle* handle,char* cstr,uint32_t timeout);


static inline void tch_uartValidate(tch_UartHandlePrototype* _handle);
static inline void tch_uartInvalidate(tch_UartHandlePrototype* _handle);
static inline BOOL tch_uartIsValid(tch_UartHandlePrototype* _handle);

__attribute__((section(".data"))) static tch_lld_uart_prototype UART_StaticInstance = {
		{
				INIT_UART_STOPBIT,
				INIT_UART_PARITY,
				MFEATURE_UART,
				tch_uartOpen,
				tch_uartClose
		},
		NULL,
		NULL,
		0,
		0
};


const tch_lld_usart* tch_usart_instance = (const tch_lld_usart*) &UART_StaticInstance;

static tch_UartHandle* tch_uartOpen(tch* env,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt){
	if(cfg->UartCh >= MFEATURE_UART)    // if requested channel is larger than uart channel count
		return NULL;                    // return null
	if(!UART_StaticInstance.mtx)
		UART_StaticInstance.mtx = env->Mtx->create();
	if(!UART_StaticInstance.condv)
		UART_StaticInstance.condv = env->Condv->create();
	if(env->Device->interrupt->isISR())
		return NULL;
	uint32_t umskb = 1 << cfg->UartCh;
	if(env->Mtx->lock(UART_StaticInstance.mtx,timeout) != osOK)
		return NULL;
	while(UART_StaticInstance.occp_state & umskb){
		if(env->Condv->wait(UART_StaticInstance.condv,UART_StaticInstance.mtx,timeout) != osOK)
			return NULL;
	}
	UART_StaticInstance.occp_state |= umskb;
	env->Mtx->unlock(UART_StaticInstance.mtx);

	tch_UartHandlePrototype* uins = env->Mem->alloc(sizeof(tch_UartHandlePrototype));
	uins->env = env;

	tch_uart_descriptor* uDesc = &UART_HWs[cfg->UartCh];
	tch_uart_bs* ubs = &UART_BD_CFGs[cfg->UartCh];
	uint8_t psc = 2;
	if(uDesc->_clkenr == (uint32_t)&RCC->APB1ENR)
		psc = 4;

	uint32_t ioclk = SYS_CLK / psc;

	float udiv = (float)ioclk / (float)cfg->Buadrate;

	int mantisa = (int)udiv;
	if(mantisa > udiv)
		mantisa--;
	float frac = udiv - (float) mantisa;
	int dfrac = (int)(udiv * 16);


	*uDesc->_clkenr |= uDesc->clkmsk;
	if(popt == ActOnSleep)
		*uDesc->_lpclkenr |= uDesc->lpclkmsk;
	USART_TypeDef* uhw = (USART_TypeDef*) uDesc->_hw;
	uhw->BRR = 0;
	uhw->BRR |= (mantisa << 4);
	uhw->BRR |= dfrac;

	uhw->CR1 = (cfg->Parity << USART_Parity_Pos_CR1) | USART_CR1_PEIE | USART_CR1_RXNEIE | USART_CR1_TE | USART_CR1_RE;
	uhw->CR2 = (cfg->StopBit << USART_StopBit_Pos_CR2);

	tch_DmaCfg dmaCfg;
	tch_lld_dma* DMA = env->Device->dma;

	if(ubs->txdma != DMA_NOT_USED){
		dmaCfg.BufferType = DMA->BufferType.Normal;
		dmaCfg.Ch = ubs->txch;
		dmaCfg.Dir = DMA->Dir.MemToPeriph;
		dmaCfg.FlowCtrl = DMA->FlowCtrl.DMA;
		dmaCfg.Priority = DMA->Priority.Normal;
		dmaCfg.mAlign = DMA->Align.Byte;
		dmaCfg.mBurstSize = DMA->BurstSize.Burst1;
		dmaCfg.mInc = TRUE;
		dmaCfg.pAlign = DMA->Align.Byte;
		dmaCfg.pBurstSize = DMA->BurstSize.Burst1;
		dmaCfg.pInc = FALSE;

		uins->txDma = DMA->allocDma(env,&dmaCfg,timeout,popt);
	}

	if(ubs->rxdma != DMA_NOT_USED){

	}


}

static void* tch_uartClose(tch_UartHandle* handle){

}


static tchStatus tch_uartPutc(tch_UartHandle* handle,uint8_t c){

}

static uint8_t tch_uartGetc(tch_UartHandle* handle,tchStatus* result){

}

static tchStatus tch_uartWrite(tch_UartHandle* handle,const uint8_t* bp,size_t sz){

}

static tchStatus tch_uartRead(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout){

}

static size_t tch_uartAvailable(tch_UartHandle* handle){

}

static tchStatus tch_uartWriteCstr(tch_UartHandle* handle,const char* cstr){

}

static tchStatus tch_uartReadCstr(tch_UartHandle* handle,char* cstr,uint32_t timeout){

}

static inline void tch_uartValidate(tch_UartHandlePrototype* _handle){

}

static inline void tch_uartInvalidate(tch_UartHandlePrototype* _handle){

}

static inline BOOL tch_uartIsValid(tch_UartHandlePrototype* _handle){

}
