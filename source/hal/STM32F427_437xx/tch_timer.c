/*
 * tch_timer.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/tch_hal.h"


typedef struct tch_lld_timer_prototype {
	tch_lld_timer                     pix;
} tch_lld_timer_prototype;


static tch_gptimerHandle* tch_lld_timer_openGpTimer(tch_timer timer,tch_gptimerDef* tdef);
static tch_pwmHandle* tch_lld_timer_openPWM(tch_timer timer,tch_pwmDef* tdef);
static tch_tcaptHandle* tch_lld_timer_openTimerCapture(tch_timer timer,tch_tcaptDef* tdef);
static void tch_lld_timer_close(tch_timer timer);


__attribute__((section(".data")))  static tch_lld_timer_prototype TIMER_StaticInstance = {
		tch_lld_timer_openGpTimer,
		tch_lld_timer_openPWM,
		tch_lld_timer_openTimerCapture
};


const tch_lld_timer* tch_timer_instance = (tch_lld_timer*) &TIMER_StaticInstance;



tch_gptimerHandle* tch_lld_timer_openGpTimer(tch_timer timer,tch_gptimerDef* tdef){

}

tch_pwmHandle* tch_lld_timer_openPWM(tch_timer timer,tch_pwmDef* tdef){

}

tch_tcaptHandle* tch_lld_timer_openTimerCapture(tch_timer timer,tch_tcaptDef* tdef){

}

void tch_lld_timer_close(tch_timer timer){

}
