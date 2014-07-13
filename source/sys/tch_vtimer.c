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




static tch_timer_id tch_timer_create(const tch_timer_def* timer_def,tch_timer_type type,void* arg);
static tchStatus tch_timer_start(tch_timer_id timer,uint32_t millisec);
static tchStatus tch_timer_stop(tch_timer_id timer);
static tchStatus tch_timer_delete(tch_timer_id timer);



__attribute__((section(".data"))) static tch_timer_ix TIMER_StaticInstance = {
		tch_timer_create,
		tch_timer_start,
		tch_timer_stop,
		tch_timer_delete
};

const tch_timer_ix* Timer = (const tch_timer_ix*)&TIMER_StaticInstance;




tch_timer_id tch_timer_create(const tch_timer_def* timer_def,tch_timer_type type,void* arg){

}

tchStatus tch_timer_start(tch_timer_id timer,uint32_t millisec){

}

tchStatus tch_timer_stop(tch_timer_id timer){

}

tchStatus tch_timer_delete(tch_timer_id timer){

}
