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
	uint32_t*            _clkenr;
	const uint32_t        clkmsk;
	uint32_t*            _lpclkenr;
	const uint32_t        lpclkmsk;
	uint32_t              io_ocpstate;
}tch_gpio_descriptor;


typedef struct _tch_ioInterrupt_descriptor {
	void*                     io_occp;
	tch_genericList_queue_t   wq;
	IRQn_Type                 irq;
}tch_ioInterrupt_descriptor;

/////////////////////////////  Platform specific mapping  ////////////////////////////


#endif /* TCH_HALDESC_H_ */
