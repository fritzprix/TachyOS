/*
 * tch_timer.c
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include "hal/STM_CMx/tch_hal.h"

typedef struct tch_lld_timer_prototype {
	tch_lld_timer                     pix;
} tch_lld_timer_prototype;



__attribute__((section(".data")))  static tch_lld_timer_prototype TIMER_StaticInstance = {

};


const tch_lld_timer* tch_timer_instance = (tch_lld_timer*) &TIMER_StaticInstance;
