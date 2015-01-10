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


/** \addtogroup Base_HAL_Interface
 *  @{
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

/**\var tch_rtc
 *  RTC HAL Object, which is not directly accessible to user program.
 *  RTC is abstracted to System Timer, which keeps track of real time information of system
 *  and also deals with timed scheduling of task
 */

/**\var tch_dma
 *
 *  DMA HAL Object, which is not directly accessible to user program.
 *  it's been recommended that DMA is accessed by another HAL Interface
 */

extern tch_lld_rtc* tch_rtc; /**<RTC HAL Object  */
extern tch_lld_dma* tch_dma; /**<DMA HAL Object  */






/**
 * \callgraph
 * \brief Enable system tick timer
 * \see tch_hal_disableSystick
 *
 * this function would be invoked in the initialization stage of system
 */
extern void tch_hal_enableSystick();

/**
 * \brief Disable system tick timer
 * \see tch_hal_enableSystick
 *
 * this function would be invoked from kernel to allow system to enter sleep mode
 */
extern void tch_hal_disableSystick();

/**
 * \brief set current low power mode
 * \param lplvl new low-power level
 * \see tch_lplvl
 *
 * this function is invoked from kernel to change system low power level
 *
 */
extern void tch_hal_setSleepMode(tch_lplvl lplvl);


/**
 * \brief Allow system to enter sleep mode
 *
 * this function is invoked from kernel to allow system to go into low power mode.
 */

extern void tch_hal_enterSleepMode();

/**
 * \brief Suspend system core clock
 *
 * this function is invoked from kernel when main clock should be turned off
 * \see tch_hal_resumeSysClock
 */

extern void tch_hal_suspendSysClock();

/**
 * \brief Resume system core clock
 *
 * this function is invoked from kernel when cpu is waken-up from sleep (low power mode)
 * \see tch_hal_suspendSysClock
 */

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

/** @}*/
