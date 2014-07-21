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

typedef enum {Rise,Fall,Both}gpio_evEdge;
typedef enum {Interrupt,Event}gpio_evType;
typedef struct tch_gpio_evCfg {
	gpio_evEdge edge;
	gpio_evType type;
}tch_gpio_evCfg;

typedef enum {Out,In,Analog,Function}gpio_mode;
typedef enum {PushPull,OpenDrain}gpio_otype;
typedef enum {IoSpeedLow,IoSpeedMid,IoSpeedHigh,IoSpeedVeryHigh}gpio_ospeed;
typedef enum {Pullup,Pulldown,NoPull}gpio_pupd;

typedef struct tch_gpio_cfg {
	gpio_mode Mode;
	gpio_otype Otype;
	gpio_ospeed OSpeed;
	gpio_pupd PuPd;
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
	BOOL (*listen)(tch_gpio_handle* self,uint32_t timeout,const tch_gpio_evCfg* cfg);
}tch_gpio_handle;


typedef struct tch_lld_gpio {
	tch_gpio_handle* (*allocIo)(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,tch_pwr_def pcfg);
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
