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


#define DECLARE_IO_CALLBACK(name) BOOL name(tch_GpioHandle* self,uint8_t pin)


typedef uint8_t gpIo_x;
typedef struct tch_gpio_handle tch_GpioHandle;
typedef BOOL (*tch_IoEventCallback_t) (tch_GpioHandle* self,uint8_t pin);


/**
 * gpio handle interface
 */

typedef struct _tch_gpio_ev_edge{
	const uint8_t Rise;
	const uint8_t Fall;
	const uint8_t Both;
}tch_gpioEvEdge;

typedef struct _tch_gpio_ports{
	const gpIo_x   gpio_0;
	const gpIo_x   gpio_1;
	const gpIo_x   gpio_2;
	const gpIo_x   gpio_3;
	const gpIo_x   gpio_4;
	const gpIo_x   gpio_5;
	const gpIo_x   gpio_6;
	const gpIo_x   gpio_7;
	const gpIo_x   gpio_8;
	const gpIo_x   gpio_9;
	const gpIo_x   gpio_10;
	const gpIo_x   gpio_11;
}tch_gpioPorts;

typedef struct _tch_gpio_ev_Type{
	const uint8_t Interrupt;
	const uint8_t Event;
}tch_gpioEvType;

typedef struct tch_gpio_evcfg_t {
	uint8_t EvEdge;
	uint8_t EvType;
	tch_IoEventCallback_t EvCallback;
	uint32_t EvTimeout;
}tch_GpioEvCfg;



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

typedef struct tch_gpio_cfg_t {
	uint8_t Mode;
	uint8_t Otype;
	uint8_t Speed;
	uint8_t PuPd;
	uint8_t Af;
}tch_GpioCfg;


typedef struct tch_gpio_handle {
	/*! \brief write gpio port's output value
	 *  \param[in] self handle object
	 *  \param[in] pmsk bit flags value to be written into gpio(s)
	 *  \param[in] nstate new state for given pin
	 */
 	void (*out)(tch_GpioHandle* self,uint32_t pmsk,tch_bState nstate);
 	/*!
 	 *
 	 */
	tch_bState (*in)(tch_GpioHandle* self,uint32_t pmsk);
	/*! \brief enable io interrupt for given io pin & register io event listener
	 *  enable interrupt for io event, pmsk should be provided which is non-zero.
	 *  if io interrupt is already used by another handle, it try to wait it to be released as long as given timeout
	 *  when timeout is expired, it return with error, marking unobtained io in pmsk parameter.
	 *
	 *  \param[in] self self instance of gpio handle
	 *  \param[in] cfg pointer of \sa tch_GpioEvCfg which is event configuration type
	 *  \param[in] pmsk pin(s) to be configured as event input, presenting as comb pattern
	 *  \param[in] timeout when requested interrupt is already in use, wait given amount of time
	 *
	 */
	tchStatus (*registerIoEvent)(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t* pmsk);

	/*!
	 *
	 */
	tchStatus (*unregisterIoEvent)(tch_GpioHandle* self,uint32_t pmsk);

	/*!
	 *
	 */
	tchStatus (*configureEvent)(tch_GpioHandle* self,const tch_GpioEvCfg* evcfg,uint32_t pmsk);

	/*!
	 *
	 */
	tchStatus (*configure)(tch_GpioHandle* self,const tch_GpioCfg* cfg,uint32_t pmsk);

	/*!
	 *
	 */
	BOOL (*listen)(tch_GpioHandle* self,uint8_t pin,uint32_t timeout);

}tch_GpioHandle;


typedef struct tch_lld_gpio {
	tch_gpioPorts Ports;
	tch_gpioMode Mode;
	tch_gpioOtype Otype;
	tch_gpioSpeed Speed;
	tch_gpioPuPd PuPd;
	tch_gpioEvEdge EvEdeg;
	tch_gpioEvType EvType;
	tch_GpioHandle* (*allocIo)(const tch* api,const gpIo_x port,uint32_t pmsk,const tch_GpioCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg);
	void (*initCfg)(tch_GpioCfg* cfg);
	void (*initEvCfg)(tch_GpioEvCfg* evcfg);
	uint16_t (*getPortCount)();
	uint16_t (*getPincount)(const gpIo_x port);
	uint32_t (*getPinAvailable)(const gpIo_x port);
	void (*freeIo)(tch_GpioHandle* IoHandle);
}tch_lld_gpio;

extern const tch_lld_gpio* tch_gpio_instance;


#if defined(__cplusplus)
}
#endif


#endif /* TCH_GPIO_H_ */
