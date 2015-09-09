/*! \brief HAL Description Type Definition Header
 *
 *  HAL Object Model is defined so that
 *
 */

/*
 * tch_haldesc.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 19.
 *      Author: innocentevil
 */

#ifndef TCH_HALDESC_H_
#define TCH_HALDESC_H_

#include "tch.h"
#include "tch_dma.h"
#include "tch_rtc.h"
#include "tch_ktypes.h"



////////////////////////////////   Device Description Header   ////////////////////////////////////


typedef struct _tch_gpio_descriptor {
	void*                _hw;
	volatile uint32_t*   _clkenr;
	const uint32_t        clkmsk;
	volatile uint32_t*   _lpclkenr;
	const uint32_t        lpclkmsk;
	uint32_t              io_ocpstate;
}tch_gpio_descriptor;


typedef struct _tch_ioInterrupt_descriptor {
	void*                     io_occp;
	tch_barId                 evbar;
	IRQn_Type                 irq;
}tch_ioInterrupt_descriptor;


typedef struct _tch_dma_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpcklmsk;
	volatile uint32_t*  _isr;
	volatile uint32_t*  _icr;
	uint32_t             ipos;
	IRQn_Type            irq;
}tch_dma_descriptor;

typedef struct _tch_uart_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _rstr;
	const uint32_t       rstmsk;
	IRQn_Type            irq;
}tch_uart_descriptor;

typedef struct _tch_timer_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*   rstr;
	const uint32_t       rstmsk;
	const uint8_t        channelCnt;
	const uint8_t        precision;
	uint8_t              ch_occp;
	IRQn_Type            irq;
}tch_timer_descriptor;

typedef struct _tch_spi_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _rstr;
	const uint32_t       rstmsk;
	IRQn_Type            irq;
}tch_spi_descriptor;

typedef struct _tch_iic_descriptor {
	void*               _hw;
	void*               _handle;
	uint8_t              sh_cnt;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _rstr;
	const uint32_t       rstmsk;
	IRQn_Type            irq;
}tch_iic_descriptor;


typedef struct _tch_adc_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpclkmsk;
	volatile uint32_t*  _rstr;
	const uint32_t       rstmsk;
	IRQn_Type            irq;
}tch_adc_descriptor;


typedef struct _tch_rtc_descriptor {
	void*               _hw;
	void*               _handle;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	IRQn_Type            irq_0;
	IRQn_Type            irq_1;
}tch_rtc_descriptor;



/////////////////////////////  Platform specific mapping  ////////////////////////////


typedef struct _tch_port_t {
	gpIo_x          port;
	uint8_t         pin;
}tch_port;

typedef struct _tch_uart_bs_t {
	dma_t          txdma;
	dma_t          rxdma;
	uint8_t        txch;
	uint8_t        rxch;
	gpIo_x         port;
	int8_t         txp;
	int8_t         rxp;
	int8_t         ctsp;
	int8_t         rtsp;
	uint8_t        afv;
}tch_uart_bs;


typedef struct _tch_timer_bs_t {
	gpIo_x         port;
	int8_t         chp[4];
	uint8_t        afv;
}tch_timer_bs;

typedef struct _tch_spi_bs_t {
	dma_t          txdma;
	dma_t          rxdma;
	uint8_t        txch;
	uint8_t        rxch;
	gpIo_x         port;
	int8_t         mosi;
	int8_t         miso;
	int8_t         sck;
	uint8_t        afv;
}tch_spi_bs;

typedef struct _tch_iic_bs_t {
	dma_t         txdma;
	dma_t         rxdma;
	uint8_t       txch;
	uint8_t       rxch;
	gpIo_x        port;
	uint8_t       scl;
	uint8_t       sda;
	uint8_t       afv;
}tch_iic_bs;

typedef struct _tch_adc_bs_t {
	dma_t         dma;
	uint8_t       dmaCh;
	tch_timer     timer;
	uint8_t       timerCh;
	uint8_t       timerExtsel;
}tch_adc_bs;

#define ADC1_Bit            ((uint8_t) 1 << 0)
#define ADC2_Bit            ((uint8_t) 1 << 1)
#define ADC3_Bit            ((uint8_t) 1 << 2)

typedef struct _tch_adc_ch_bs_t {
	tch_port      port;
	uint8_t       adc_routemsk;
	uint8_t       occp;
}tch_adc_channel_bs;




extern __TCH_STATIC_INIT tch_gpio_descriptor GPIO_HWs[MFEATURE_GPIO];
extern __TCH_STATIC_INIT tch_ioInterrupt_descriptor IoInterrupt_HWs[MFEATURE_PINCOUNT_pPORT];
extern __TCH_STATIC_INIT tch_dma_descriptor DMA_HWs[MFEATURE_DMA];
extern __TCH_STATIC_INIT tch_uart_descriptor UART_HWs[MFEATURE_UART];
extern __TCH_STATIC_INIT tch_timer_descriptor TIMER_HWs[MFEATURE_TIMER];
extern __TCH_STATIC_INIT tch_spi_descriptor SPI_HWs[MFEATURE_SPI];
extern __TCH_STATIC_INIT tch_iic_descriptor IIC_HWs[MFEATURE_IIC];
extern __TCH_STATIC_INIT tch_adc_descriptor ADC_HWs[MFEATURE_ADC];




#endif /* TCH_HALDESC_H_ */
