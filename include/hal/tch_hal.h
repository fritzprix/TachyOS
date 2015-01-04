/*
 * tch_hal.h
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

#ifndef TCH_HAL_H_
#define TCH_HAL_H_

#if defined(__cplusplus)
extern "C"{
#endif

#include "tch_halcfg.h"
#include "tch_haldesc.h"

#include "tch_rtc.h"
#include "tch_dma.h"

typedef enum {
	LP_LEVEL0 = ((uint8_t) 0),
	LP_LEVEL1 = ((uint8_t) 1),
	LP_LEVEL2 = ((uint8_t) 2)
}tch_lplvl;

/*-------------------- Private hal member declaration ----------------------------- */
/*--------------RTC & DMA are not allowed to be exposed to user ------------------- */
/*---------------1. DMA potentially produces serious security flaws --------------- */
/*---------------2. RTC provides critical functionality in system low power mode--- */
/*--------------------------------------------------------------------------------- */

extern tch_lld_rtc* tch_rtc;
extern tch_lld_dma* tch_dma;


extern void tch_hal_enableSystick();
extern void tch_hal_disableSystick();
extern void tch_hal_setSleepMode(tch_lplvl lplvl);
extern void tch_hal_enterSleepMode();
extern void tch_hal_suspendSysClock();
extern void tch_hal_resumeSysClock();


extern tch_lld_adc* tch_adcHalInit(const tch* env);
extern tch_lld_dma* tch_dmaHalInit(const tch* env);
extern tch_lld_gpio* tch_gpioHalInit(const tch* env);
extern tch_lld_iic* tch_iicHalInit(const tch* env);
extern tch_lld_rtc* tch_rtcHalInit(const tch* env);
extern tch_lld_spi* tch_spiHalInit(const tch* env);
extern tch_lld_timer* tch_timerHalInit(const tch* env);
extern tch_lld_usart* tch_usartHalInit(const tch* env);


#if defined(__cplusplus)
}
#endif
#endif /* TCH_HAL_H_ */
