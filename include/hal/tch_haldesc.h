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
	tch_lnode_t               wq;
	IRQn_Type                 irq;
}tch_ioInterrupt_descriptor;


/**
 * 	DMA_Stream_TypeDef* _hw;
	uint16_t             status;
	volatile uint32_t*  _isr;
	volatile uint32_t*  _icr;
	const uint32_t       ipos;
	IRQn_Type            irq;
 */
typedef struct _tch_dma_descriptor {
	void*               _hw;
	volatile uint32_t*  _clkenr;
	const uint32_t       clkmsk;
	volatile uint32_t*  _lpclkenr;
	const uint32_t       lpcklmsk;
	volatile uint32_t*  _isr;
	volatile uint32_t*  _icr;
	uint32_t             ipos;
	IRQn_Type            irq;
}tch_dma_descriptor;

/////////////////////////////  Platform specific mapping  ////////////////////////////


#endif /* TCH_HALDESC_H_ */
