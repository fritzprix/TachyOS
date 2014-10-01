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

typedef struct tch_lld_uart_handle_t tch_UartHandle;
typedef struct tch_lld_uart_cfg_t tch_UartCfg;


struct _tch_lld_uart_cfg_t {
	uint8_t UartCh;
	uint8_t StopBit;
	uint8_t Parity;
	uint32_t Buadrate;
};



struct _tch_lld_uart_stopbit_t{
	uint8_t StopBit0dot5B;
	uint8_t StopBit1B;
	uint8_t StopBit1dot5B;
	uint8_t StopBit2B;
};

struct _tch_lld_uart_parity_t{
	uint8_t Parity_Non;
	uint8_t Parity_Odd;
	uint8_t Parity_Even;
};

struct tch_lld_uart_handle_t{
	tchStatus (*putc)(tch_UartHandle* handle,uint8_t c);
	uint8_t (*getc)(tch_UartHandle* handle,tchStatus* result);
	tchStatus (*write)(tch_UartHandle* handle,const uint8_t* bp,size_t sz);
	tchStatus (*read)(tch_UartHandle* handle,uint8_t* bp, size_t sz,uint32_t timeout);
	size_t (*available)(tch_UartHandle* handle);
	tchStatus (*writeCstr)(tch_UartHandle* handle,const char* cstr);
	tchStatus (*readCstr)(tch_UartHandle* handle,char* cstr,uint32_t timeout);
};


typedef struct tch_lld_usart {
	const struct _tch_lld_uart_stopbit_t StopBit;
	const struct _tch_lld_uart_parity_t Parity;
	const uint8_t PortCount;
	tch_UartHandle* (*open)(tch_UartCfg* cfg,tch_PwrOpt popt);
	void* (*close)(tch_UartHandle* handle);
}tch_lld_usart;

extern const tch_lld_usart* tch_usart_instance;

#endif /* TCH_USART_H_ */
