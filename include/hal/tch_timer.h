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


#define tch_TIMER0               ((tch_timer) 0)
#define tch_TIMER1               ((tch_timer) 1)
#define tch_TIMER2               ((tch_timer) 2)
#define tch_TIMER3               ((tch_timer) 3)
#define tch_TIMER4               ((tch_timer) 4)
#define tch_TIMER5               ((tch_timer) 5)
#define tch_TIMER6               ((tch_timer) 6)
#define tch_TIMER7               ((tch_timer) 7)
#define tch_TIMER8               ((tch_timer) 8)
#define tch_TIMER9               ((tch_timer) 9)



#define TIMER_UNITTIME_mSEC      ((uint8_t) 0)
#define TIMER_UNITTIME_uSEC      ((uint8_t) 1)
#define TIMER_UNITTIME_nSEC      ((uint8_t) 2)

#define TIMER_DEFTYPE_FREQ       ((uint8_t) 0)
#define TIMER_DEFTYPE_TIME       ((uint8_t) 1)

#define TIMER_POLARITY_POSITIVE  ((uint8_t) 1)
#define TIMER_POLARITY_NEGATIVE  ((uint8_t) 0xFF)

typedef uint8_t tch_timer;

typedef struct _tch_gptimer_handle_t tch_gptimerHandle;
typedef struct _tch_gptimer_def_t tch_gptimerDef;

typedef struct _tch_pwm_handle_t tch_pwmHandle;
typedef struct _tch_pwm_def_t tch_pwmDef;

typedef struct _tch_tcapt_handle_t tch_tcaptHandle;
typedef struct _tch_tcapt_def_t tch_tcaptDef;




struct _tch_gptimer_handle_t {
	tchStatus (*close)(tch_gptimerHandle* self);
	BOOL (*wait)(tch_gptimerHandle* self,uint32_t usec);
};

struct _tch_pwm_handle_t {
	tchStatus (*close)(tch_pwmHandle* self);
	tchStatus (*start)(tch_pwmHandle* self);
	tchStatus (*stop)(tch_pwmHandle* self);
	tchStatus (*write)(tch_pwmHandle* self,uint32_t ch,float* fduty,size_t sz);
	tchStatus (*setOutputEnable)(tch_pwmHandle* self,uint8_t ch,BOOL enable,uint32_t timeout);
	BOOL (*setDuty)(tch_pwmHandle* self,uint32_t ch,float duty);
	float (*getDuty)(tch_pwmHandle* self,uint32_t ch);
};

struct _tch_tcapt_handle_t {
	tchStatus (*close)(tch_tcaptHandle* self);
	tchStatus (*setInputEnable)(tch_tcaptHandle* self,uint8_t ch,BOOL enable,uint32_t timeout);
	tchStatus (*read)(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout);
};


struct _tch_gptimer_def_t {
	uint8_t UnitTime;
	tch_PwrOpt pwrOpt;
};

struct _tch_pwm_def_t {
	uint8_t UnitTime;
	uint16_t PeriodInUnitTime;
	tch_PwrOpt pwrOpt;
};



struct _tch_tcapt_def_t {
	uint8_t Polarity;
	uint8_t UnitTime;
	uint16_t periodInUnitTime;
	tch_PwrOpt pwrOpt;
};

typedef struct tch_lld_timer {
	const uint16_t TIMER_COUNT;
	tch_gptimerHandle* (*openGpTimer)(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def,uint32_t timeout);
	tch_pwmHandle* (*openPWM)(const tch* env,tch_timer timer,tch_pwmDef* tdef,uint32_t timeout);
	tch_tcaptHandle* (*openTimerCapture)(const tch* env,tch_timer timer,tch_tcaptDef* tdef,uint32_t timeout);
	uint32_t (*getChannelCount)(tch_timer timer);
	uint8_t (*getPrecision)(tch_timer timer);
} tch_lld_timer;


#endif /* TCH_TIMER_H_ */
