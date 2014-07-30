/*
 * tch_gpio.h
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

#ifndef TCH_GPIO_H_
#define TCH_GPIO_H_

#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum {
	gpIo_0 = ((uint8_t) 0),
	gpIo_1, gpIo_2, gpIo_3, gpIo_4, gpIo_5, gpIo_6, gpIo_7, gpIo_8, gpIo_9, gpIo_10,
	gpIo_11 = ((uint8_t) 11)
}gpIo_x;
typedef struct tch_gpio_handle tch_gpio_handle;

/**
 * gpio handle interface
 */

typedef struct _tch_gpio_ev_edge{
	const uint8_t Rise;
	const uint8_t Fall;
	const uint8_t Both;
}tch_gpioEvEdge;

typedef struct _tch_gpio_ev_Type{
	const uint8_t Interrupt;
	const uint8_t Event;
}tch_gpioEvType;

typedef struct tch_gpio_evCfg {
	uint8_t EvEdge;
	uint8_t EvType;
}tch_gpio_evCfg;



typedef struct _tch_gpio_mode_t {
	const uint8_t Out;
	const uint8_t In;
	const uint8_t Analog;
	const uint8_t Func;
}tch_gpioMode;

typedef struct _tch_gpio_otype_t {
	const uint8_t PushPull;
	const uint8_t OpenDrain;
}tch_gpioOtype;

typedef struct _tch_gpio_ospeed_t{
	const uint8_t Low;
	const uint8_t Mid;
	const uint8_t High;
	const uint8_t VeryHigh;
}tch_gpioSpeed;

typedef struct _tch_gpio_pupd_t{
	const uint8_t PullUp;
	const uint8_t PullDown;
	const uint8_t NoPull;
}tch_gpioPuPd;

typedef struct tch_gpio_cfg {
	uint8_t Mode;
	uint8_t Otype;
	uint8_t Speed;
	uint8_t PuPd;
	uint8_t Af;
}tch_gpio_cfg;

typedef BOOL (*tch_IoEventCalback_t) (tch_gpio_handle* self,uint8_t pin);

typedef struct tch_gpio_handle {
	/**
	 * write gpio port's output value
	 * @param self handle object
	 * @param val bit flags value to be written into gpio(s)
	 */
	void (*out)(tch_gpio_handle* self,tch_bState nstate);
	tch_bState (*in)(tch_gpio_handle* self);
	tchStatus (*registerIoEvent)(tch_gpio_handle* self,const tch_gpio_evCfg* cfg,const tch_IoEventCalback_t callback);
	tchStatus (*unregisterIoEvent)(tch_gpio_handle* self);
	tchStatus (*configure)(tch_gpio_handle* self,const tch_gpio_evCfg* cfg);
	BOOL (*listen)(tch_gpio_handle* self,uint32_t timeout);
}tch_gpio_handle;


typedef struct tch_lld_gpio {
	tch_gpioMode Mode;
	tch_gpioOtype Otype;
	tch_gpioSpeed Speed;
	tch_gpioPuPd PuPd;
	tch_gpioEvEdge EvEdeg;
	tch_gpioEvType EvType;
	tch_gpio_handle* (*allocIo)(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,uint32_t timeout,tch_pwr_def pcfg);
	void (*initCfg)(tch_gpio_cfg* cfg);
	void (*initEvCfg)(tch_gpio_evCfg* evcfg);
	uint16_t (*getPortCount)();
	uint16_t (*getPincount)(const gpIo_x port);
	uint32_t (*getPinAvailable)(const gpIo_x port);
	void (*freeIo)(tch_gpio_handle* IoHandle);
}tch_lld_gpio;

extern const tch_lld_gpio* tch_gpio_instance;


#if defined(__cplusplus)
}
#endif


#endif /* TCH_GPIO_H_ */
