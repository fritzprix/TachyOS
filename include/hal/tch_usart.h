/*
 * tch_usart.h
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
 */

#ifndef TCH_USART_H_
#define TCH_USART_H_

#include "tch.h"

#if defined(_cplusplus)
extern "C"{
#endif


#define tch_USART0                          ((uart_t) 0)
#define tch_USART1                          ((uart_t) 1)
#define tch_USART2                          ((uart_t) 2)
#define tch_USART3                          ((uart_t) 3)
#define tch_USART4                          ((uart_t) 4)
#define tch_USART5                          ((uart_t) 5)
#define tch_USART6                          ((uart_t) 6)
#define tch_USART7                          ((uart_t) 7)


#define USART_StopBit_1B                    (uint8_t) (0)
#define USART_StopBit_0dot5B                (uint8_t) (1)
#define USART_StopBit_2B                    (uint8_t) (2)
#define USART_StopBit_1dot5B                (uint8_t) (3)

#define USART_Parity_NON                    (uint8_t) (0 << 1 | 0 << 0)
#define USART_Parity_ODD                    (uint8_t) (1 << 1 | 1 << 0)
#define USART_Parity_EVEN                   (uint8_t) (1 << 1 | 0 << 0)


typedef uint8_t uart_t;
typedef struct _tch_lld_uart_handle_t tch_UartHandle;
typedef struct _tch_lld_uart_cfg_t tch_UartCfg;


struct _tch_lld_uart_cfg_t {
	uint8_t    StopBit;
	uint8_t    Parity;
	uint32_t   Buadrate;
	BOOL       FlowCtrl;
};


struct _tch_lld_uart_handle_t{
	tchStatus (*close)(tch_UartHandle* handle);
	tchStatus (*putc)(tch_UartHandle* handle,uint8_t c);
	tchStatus (*getc)(tch_UartHandle* handle,uint8_t* rc,uint32_t timeout);
	tchStatus (*write)(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
	tchStatus (*read)(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);
};


typedef struct tch_lld_usart {
	const uint8_t PORT_COUNT;
	tch_UartHandle* (*allocUart)(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
}tch_lld_usart;

extern tch_lld_usart* tch_usartHalInit(const tch* env);

#if defined(_cplusplus)
}
#endif


#endif /* TCH_USART_H_ */
