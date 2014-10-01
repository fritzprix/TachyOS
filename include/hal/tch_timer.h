/*
 * tch_timer.h
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

#ifndef TCH_TIMER_H_
#define TCH_TIMER_H_

#include "tch.h"

typedef uint32_t tch_timer;

typedef struct _tch_gptimer_handle_t tch_gptimer_handle;
typedef struct _tch_gptimer_def_t tch_gptimer_def;

typedef struct _tch_pwm_handle_t tch_pwm_handle;
typedef struct _tch_pwm_def_t tch_pwm_def;

typedef struct _tch_tcapt_handle_t tch_tcapt_handle;
typedef struct _tch_tcapt_def_t tch_tcapt_def;

struct _tch_gptimer_handle_t {
	BOOL (*start)(tch_gptimer_handle* self,uint32_t millisec);
	BOOL (*stop)(tch_gptimer_handle* self);
};

struct _tch_gptimer_def_t {
	uint32_t            unit_us;
	tch_timer_callback  callback;
	tch_PwrOpt         pwr_cfg;
};

struct _tch_pwm_handle_t {
	BOOL (*start)(tch_pwm_handle* self,uint32_t ch);
	BOOL (*stop)(tch_pwm_handle* self,uint32_t ch);
	uint32_t (*getChCount)(tch_pwm_handle* self);
	uint32_t (*setDuty)(tch_pwm_handle* self,uint32_t ch);
	uint32_t (*getDuty)(tch_pwm_handle* self,uint32_t ch);
};

struct _tch_pwm_def_t {
	uint32_t          period_us;
	tch_PwrOpt       pwr_cfg;
};

struct _tch_tcapt_handle_t {

};

struct _tch_tcapt_def_t {

};

typedef struct tch_lld_timer {
	tch_gptimer_handle* (*openGpTimer)(tch_timer timer,tch_gptimer_def* tdef);
	tch_pwm_handle* (*openPWM)(tch_timer timer,tch_pwm_def* tdef);
	tch_tcapt_handle* (*openTimerCapture)(tch_timer timer,tch_tcapt_def* tdef);
	void (*close)(tch_timer timer);
}tch_lld_timer;

extern const tch_lld_timer* tch_timer_instance;

#endif /* TCH_TIMER_H_ */
