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


/** \addtogroup hal_api HAL API
 *  @{
 */

#ifndef TCH_HAL_H_
#define TCH_HAL_H_

#include "tch_halcfg.h"
#include "tch_haldesc.h"

#include "hal/tch_rtc.h"
#include "hal/tch_dma.h"

#if defined(__cplusplus)
extern "C"{
#endif


/*!
 * \addtogroup hal_api HAL API
 */

/**
 * An enum for defining low power level
 */
typedef enum {
	LP_LEVEL0 = ((uint8_t) 0),   /**< CPU Idle : CPU clock source is gated */
	LP_LEVEL1 = ((uint8_t) 1),   /**< CPU Sleep : CPU clock is turned off, contents in SRAM are kept */
	LP_LEVEL2 = ((uint8_t) 2)    /**< CPU Deep Sleep : CPU clock is turned off, contents in SRAM are lost */
}tch_lplvl;

/*-------------------- Private hal member declaration ----------------------------- */
/*--------------RTC & DMA are not allowed to be exposed to user ------------------- */
/*---------------1. DMA potentially produces serious security flaws --------------- */
/*---------------2. RTC provides critical functionality in system low power mode--- */
/*--------------------------------------------------------------------------------- */



/*!
 * \brief Enable system tick timer
 * \see tch_hal_disableSystick
 *
 * this function would be invoked in the initialization stage of system
 */
extern void tch_hal_enableSystick(uint32_t mills);

/*!
 * \brief Disable system tick timer
 * \see tch_hal_enableSystick
 *
 * this function would be invoked from kernel to allow system to enter sleep mode
 */
extern void tch_hal_disableSystick();

/*!
 * \brief set current low power mode
 * \param lplvl new low-power level
 * \see tch_lplvl
 *
 * this function is invoked from kernel to change system low power level
 *
 */
extern void tch_hal_setSleepMode(tch_lplvl lplvl);


/*!
 * \brief Allow system to enter sleep mode
 *
 * this function is invoked from kernel to allow system to go into low power mode.
 */

extern void tch_hal_enterSleepMode();

/*!
 * \brief pause system core clock
 *
 * this function is invoked from kernel when main clock should be turned off
 * \see tch_hal_resumeSysClock
 */

extern void tch_hal_pauseSysClock();

/*!
 * \brief Resume system core clock
 *
 * this function is invoked from kernel when cpu is waken-up from sleep (low power mode)
 * \see tch_hal_suspendSysClock
 */

extern void tch_hal_resumeSysClock();


#if defined(__cplusplus)
}
#endif
#endif /* TCH_HAL_H_ */

/** @}*/
