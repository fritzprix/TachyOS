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


/*!
 * \addtogroup hal_api
 * @{
 */

/*!
 * \addtogroup gpio_api GPIO API
 * @{
 */

#ifndef TCH_GPIO_H_
#define TCH_GPIO_H_

#include "tch_types.h"

#if defined(__cplusplus)
extern "C" {
#endif



#define tch_gpio0              ((gpIo_x) 0)   ///< 1st GPIO port (can be PORT A or PORT 0 depend on hardware)
#define tch_gpio1              ((gpIo_x) 1)   ///< 2nd GPIO port
#define tch_gpio2              ((gpIo_x) 2)   ///< 3rd GPIO port
#define tch_gpio3              ((gpIo_x) 3)   ///< 4th GPIO port
#define tch_gpio4              ((gpIo_x) 4)   ///< 5th GPIO port
#define tch_gpio5              ((gpIo_x) 5)   ///< 6th GPIO port
#define tch_gpio6              ((gpIo_x) 6)   ///< 7th GPIO port
#define tch_gpio7              ((gpIo_x) 7)   ///< 8th GPIO port
#define tch_gpio8              ((gpIo_x) 8)   ///< 9th GPIO port
#define tch_gpio9              ((gpIo_x) 9)   ///< 10th GPIO port
#define tch_gpio10             ((gpIo_x) 10)  ///< 11th GPIO port
#define tch_gpio11             ((gpIo_x) 11)  ///< 12th GPIO port


#define GPIO_Mode_IN                   (uint8_t) (0)
#define GPIO_Mode_OUT                  (uint8_t) (1)
#define GPIO_Mode_AF                   (uint8_t) (2)
#define GPIO_Mode_AN                   (uint8_t) (3)

#define GPIO_Otype_PP                   (uint8_t) (0)
#define GPIO_Otype_OD                   (uint8_t) (1)


#define GPIO_OSpeed_LEVEL0               (uint8_t) (0)
#define GPIO_OSpeed_LEVEL1               (uint8_t) (1)
#define GPIO_OSpeed_LEVEL2               (uint8_t) (2)
#define GPIO_OSpeed_LEVEL3               (uint8_t) (3)

#define GPIO_PuPd_Float                 (uint8_t) (0)
#define GPIO_PuPd_PU                    (uint8_t) (1)
#define GPIO_PuPd_PD                    (uint8_t) (2)

#define GPIO_EvEdge_Rise               ((uint8_t) 1)
#define GPIO_EvEdge_Fall               ((uint8_t) 2)

#define GPIO_EvType_Interrupt          ((uint8_t) 1)
#define GPIO_EvType_Event              ((uint8_t) 2)


#define DECLARE_IO_CALLBACK(name) BOOL name(tch_GpioHandle* self,uint8_t pin)


typedef uint8_t gpIo_x;
typedef uint8_t pin;
typedef struct tch_gpio_handle tch_gpioHandle;
typedef BOOL (*tch_IoEventCallback_t) (tch_gpioHandle* self,uint8_t pin);



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


typedef struct tch_gpio_evcfg_t {
	uint8_t EvEdge;
	uint8_t EvType;
	tch_IoEventCallback_t EvCallback;
}gpio_event_config_t;



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
	uint8_t    Mode;
	uint8_t    Otype;
	uint8_t    Speed;
	uint8_t    PuPd;
	uint8_t    Af;
	tch_PwrOpt popt;
}gpio_config_t;


struct tch_gpio_handle {
	tchStatus (*close)(tch_gpioHandle* IoHandle);
	/*! \brief write gpio port's output value
	 *  \param[in] self handle object
	 *  \param[in] pmsk bit flags value to be written into gpio(s)
	 *  \param[in] nstate new state for given pin
	 */
 	void (*out)(tch_gpioHandle* self,uint32_t pmsk,tch_bState nstate);
 	/*!
 	 *
 	 */
	tch_bState (*in)(tch_gpioHandle* self,uint32_t pmsk);
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
	tchStatus (*registerIoEvent)(tch_gpioHandle* self,pin p,const gpio_event_config_t* cfg);

	/*!
	 *
	 */
	tchStatus (*unregisterIoEvent)(tch_gpioHandle* self,pin p);

	/*!
	 *
	 */
	tchStatus (*configureEvent)(tch_gpioHandle* self,pin p,const gpio_event_config_t* evcfg);

	/*!
	 *
	 */
	tchStatus (*configure)(tch_gpioHandle* self,const gpio_config_t* cfg,uint32_t pmsk);

	/*!
	 *
	 */
	BOOL (*listen)(tch_gpioHandle* self,pin pin,uint32_t timeout);

};


typedef struct tch_hal_module_gpio {
	const uint16_t count;
	tch_gpioHandle* (*allocIo)(const tch_core_api_t* api,const gpIo_x port,uint32_t pmsk,const gpio_config_t* cfg,uint32_t timeout);
	void (*initCfg)(gpio_config_t* cfg);
	void (*initEvCfg)(gpio_event_config_t* evcfg);
} tch_hal_module_gpio_t;


#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

/*!
 * @}
 */
#endif /* TCH_GPIO_H_ */
