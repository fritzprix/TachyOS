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


#include "hal/tch_hal.h"
#include "tch_dma.h"


#define TCH_UART_MAX_COUNT     ((uint8_t) 3)
#define TCH_UART_CLASS_KEY     ((uint16_t) 0x3D02)

#define tch_uartValidate(uart)
#define tch_uartIsValid(uart)
#define tch_uartInvalidate(uart)




#define USART_Parity_Pos_CR1                (uint8_t) (9)
#define USART_Parity_NON                    (uint8_t) (0 << 1 | 0 << 0)
#define USART_Parity_ODD                    (uint8_t) (1 << 1 | 1 << 0)
#define USART_Parity_EVEN                   (uint8_t) (1 << 1 | 0 << 0)

#define USART_StopBit_Pos_CR2               (uint8_t) (12)
#define USART_StopBit_1B                    (uint8_t) (0)
#define USART_StopBit_0dot5B                (uint8_t) (1)
#define USART_StopBit_2B                    (uint8_t) (2)
#define USART_StopBit_1dot5B                (uint8_t) (3)




static tch_UartHandle* tch_uartOpen(tch_UartCfg* cfg,tch_PwrOpt popt);
static void* tch_uartClose(tch_UartHandle* handle);

static tchStatus tch_uartPutc(tch_UartHandle* handle,uint8_t c);
static uint8_t tch_uartGetc(tch_UartHandle* handle,tchStatus* result);
static tchStatus tch_uartWrite(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
static tchStatus tch_uartRead(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);
static size_t tch_uartAvailable(tch_UartHandle* handle);
static tchStatus tch_uartWriteCstr(tch_UartHandle* handle,const char* cstr);
static tchStatus tch_uartReadCstr(tch_UartHandle* handle,char* cstr,uint32_t timeout);



typedef struct tch_lld_usart_handle_prototype_t {
	tch_UartHandle                    pix;
	tch_DmaHandle*                    txDma;
	tch_DmaHandle*                    rxDma;
	tch_GpioHandle*                   rxPin;
	tch_GpioHandle*                   txPin;
	tch_GpioHandle*                   ctsPin;
	tch_GpioHandle*                   rtsPin;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint32_t                          status;
}tch_UartHandlePrototype;

typedef struct tch_lld_usart_prototype {
	tch_lld_usart                     pix;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
	uint16_t                          occp_state;
	uint16_t                          lpoccp_state;
}tch_lld_usart_prototype;

__attribute__((section(".data"))) static tch_lld_usart_prototype USART_StaticInstance = {

};


const tch_lld_usart* tch_usart_instance = (const tch_lld_usart*) &USART_StaticInstance;
