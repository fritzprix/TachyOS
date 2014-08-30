/*
 * tch_systimer.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_VTIMER_H_
#define TCH_VTIMER_H_

#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif

/***
 *  timer types
 */
typedef void* tch_timer_id;
typedef struct _tch_timer_def_t tch_timer_def;
typedef void* (*tch_timer_callback)(void* arg);


typedef enum {
	Once,Periodic
}tch_timer_type;


struct _tch_timer_def_t {
	tch_timer_callback     fn;
};

struct _tch_timer_ix_t {
	tch_timer_id (*create)(const tch_timer_def* timer_def,tch_timer_type type,void* arg);
	tchStatus (*start)(tch_timer_id timer,uint32_t millisec);
	tchStatus (*stop)(tch_timer_id timer);
	tchStatus (*destroy)(tch_timer_id timer);
};


#if defined(__cplusplus)
}
#endif

#endif /* TCH_SYSTIMER_H_ */
