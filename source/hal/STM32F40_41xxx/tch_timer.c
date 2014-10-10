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


#define TIMER_UNITTIME_mSEC      ((uint8_t) 0)
#define TIMER_UNITTIME_uSEC      ((uint8_t) 1)
#define TIMER_UNITTIME_nSEC      ((uint8_t) 2)

#define INIT_UNTITIME_STR        {\
	                              TIMER_UNITTIME_mSEC,\
	                              TIMER_UNITTIME_uSEC,\
	                              TIMER_UNITTIME_nSEC\
}


typedef struct tch_timer_mgr_t {
	struct _tch_timer_utime_t         UnitTime;
	tch_lld_timer                     pix;
} tch_timer_manager;



///////            Timer Manager Function               ///////
static tch_gptimerHandle* tch_timer_openGpUnit(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def);
static tch_pwmHandle* tch_timer_openPWM(const tch* env,tch_timer timer,tch_pwmDef* tdef);
static tch_tcaptHandle* tch_timer_openCapture(const tch* env,tch_timer timer,tch_tcaptDef* tdef);
static uint32_t tch_timer_getChannelCount(tch_timer timer);
static uint8_t tch_timer_getPrecision(tch_timer timer);


//////         General Purpose Timer Function           ///////
static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t usec);
static tchStatus tch_gptimer_close(tch_gptimerHandle* self);


//////            PWM fucntion                        //////
static BOOL tch_pwm_start(tch_pwmHandle* self,uint32_t ch);
static BOOL tch_pwm_stop(tch_pwmHandle* self,uint32_t ch);
static uint32_t tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch);
static uint32_t tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch);
static tchStatus tch_pwm_close(tch_pwmHandle* self);


/////             Pulse Capture Function                /////
static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout);
static tchStatus tch_tcapt_close(tch_tcaptHandle* self);


__attribute__((section(".data")))  static tch_timer_manager TIMER_StaticInstance = {
		INIT_UNTITIME_STR,
		tch_timer_openGpUnit,
		tch_timer_openPWM,
		tch_timer_openCapture,
		tch_timer_getChannelCount,
		tch_timer_getPrecision
};


const tch_lld_timer* tch_timer_instance = (tch_lld_timer*) &TIMER_StaticInstance;

///////            Timer Manager Function               ///////
static tch_gptimerHandle* tch_timer_openGpUnit(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def){

}

static tch_pwmHandle* tch_timer_openPWM(const tch* env,tch_timer timer,tch_pwmDef* tdef){

}

static tch_tcaptHandle* tch_timer_openCapture(const tch* env,tch_timer timer,tch_tcaptDef* tdef){

}

static uint32_t tch_timer_getChannelCount(tch_timer timer){

}

static uint8_t tch_timer_getPrecision(tch_timer timer){

}


//////         General Purpose Timer Function           ///////
static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t usec){

}

static tchStatus tch_gptimer_close(tch_gptimerHandle* self){

}


//////            PWM fucntion                        //////
static BOOL tch_pwm_start(tch_pwmHandle* self,uint32_t ch){

}

static BOOL tch_pwm_stop(tch_pwmHandle* self,uint32_t ch){

}

static uint32_t tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch){

}

static uint32_t tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch){

}

static tchStatus tch_pwm_close(tch_pwmHandle* self){

}


/////             Pulse Capture Function                /////
static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout){

}

static tchStatus tch_tcapt_close(tch_tcaptHandle* self){

}
