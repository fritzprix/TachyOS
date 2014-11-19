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

typedef uint8_t tch_timer;

typedef struct _tch_gptimer_handle_t tch_gptimerHandle;
typedef struct _tch_gptimer_def_t tch_gptimerDef;

typedef struct _tch_pwm_handle_t tch_pwmHandle;
typedef struct _tch_pwm_def_t tch_pwmDef;

typedef struct _tch_tcapt_handle_t tch_tcaptHandle;
typedef struct _tch_tcapt_def_t tch_tcaptDef;


struct _tch_timer_str_t {
	tch_timer    timer0;
	tch_timer    timer1;
	tch_timer    timer2;
	tch_timer    timer3;
	tch_timer    timer4;
	tch_timer    timer5;
	tch_timer    timer6;
	tch_timer    timer7;
	tch_timer    timer8;
	tch_timer    timer9;
};

struct _tch_gptimer_handle_t {
	tchStatus (*close)(tch_gptimerHandle* self);
	BOOL (*wait)(tch_gptimerHandle* self,uint32_t usec);
};

struct _tch_pwm_handle_t {
	tchStatus (*close)(tch_pwmHandle* self);
	tchStatus (*write)(tch_pwmHandle* self,uint32_t ch,float* fduty,size_t sz);
	BOOL (*setDuty)(tch_pwmHandle* self,uint32_t ch,float duty);
	float (*getDuty)(tch_pwmHandle* self,uint32_t ch);
};

struct _tch_tcapt_handle_t {
	tchStatus (*close)(tch_tcaptHandle* self);
	tchStatus (*read)(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout);
};

struct _tch_timer_utime_t{
	uint8_t mSec;
	uint8_t uSec;
	uint8_t nSec;
};


struct _tch_timer_polarity_t{
	uint8_t positive;
	uint8_t negative;
};

struct _tch_gptimer_def_t {
	uint8_t UnitTime;
	tch_PwrOpt pwrOpt;
};

struct _tch_pwm_def_t {
	uint8_t UnitTime;
	uint32_t PeriodInUnitTime;
	tch_PwrOpt pwrOpt;
};



struct _tch_tcapt_def_t {
	uint8_t Polarity;
	uint8_t UnitTime;
	uint32_t periodInUnitTime;
	tch_PwrOpt pwrOpt;
};

typedef struct tch_lld_timer {
	struct _tch_timer_str_t timer;
	struct _tch_timer_utime_t UnitTime;
	struct _tch_timer_polarity_t Polarity;
	tch_gptimerHandle* (*openGpTimer)(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def,uint32_t timeout);
	tch_pwmHandle* (*openPWM)(const tch* env,tch_timer timer,tch_pwmDef* tdef,uint32_t timeout);
	tch_tcaptHandle* (*openTimerCapture)(const tch* env,tch_timer timer,tch_tcaptDef* tdef,uint32_t timeout);
	uint32_t (*getChannelCount)(tch_timer timer);
	uint8_t (*getPrecision)(tch_timer timer);
} tch_lld_timer;

extern const tch_lld_timer* tch_timer_instance;

#endif /* TCH_TIMER_H_ */
