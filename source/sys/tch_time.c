/*
 * tch_vtimer.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 30.
 *      Author: manics99
 */


#include "tch_kernel.h"
#include "tch_time.h"



static uint64_t tch_systime_setCurrentTimeMills(uint64_t time);
static uint64_t tch_systime_getCurrentTimeMills();
static uint64_t tch_systime_currentThreadTimeMills();
static uint64_t tch_systime_elapsedRealtime();
static uint64_t tch_systime_uptimeMills();


__attribute__((section(".data"))) static tch_systime_ix TIMER_StaticInstance = {
		tch_systime_setCurrentTimeMills,
		tch_systime_getCurrentTimeMills,
		tch_systime_currentThreadTimeMills,
		tch_systime_elapsedRealtime,
		tch_systime_uptimeMills
};

const tch_systime_ix* Timer = (const tch_systime_ix*)&TIMER_StaticInstance;


tch_systime_ix* tch_systimeInit(uint64_t timeInMills){

}



static uint64_t tch_systime_setCurrentTimeMills(uint64_t time){

}

static uint64_t tch_systime_getCurrentTimeMills(){

}

static uint64_t tch_systime_currentThreadTimeMills(){

}

static uint64_t tch_systime_elapsedRealtime(){

}

static uint64_t tch_systime_uptimeMills(){

}
