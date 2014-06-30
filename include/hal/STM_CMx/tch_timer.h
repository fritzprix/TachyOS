/*
 * tch_timer.h
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_TIMER_H_
#define TCH_TIMER_H_

typedef uint32_t tch_timer;

typedef struct _tch_gptimer_handle_t tch_gptimer_handle;
typedef struct _tch_gptimer_def_t tch_gptimer_def;

typedef struct _tch_pwm_handle_t tch_pwm_handle;
typedef struct _tch_pwm_def_t tch_pwm_def;

typedef struct _tch_tcapt_handle_t tch_tcapt_handle;
typedef struct _tch_tcapt_def_t tch_tcapt_def;

struct _tch_gptimer_handle_t {

};

struct _tch_gptimer_def_t {

};

struct _tch_pwm_handle_t {

};

struct _tch_pwm_def_t {

};

struct _tch_tcapt_handle_t {

};

struct _tch_tcapt_def_t {

};

typedef struct tch_lld_timer {
	tch_gptimer_handle* (*openAsTimer)(tch_timer timer,tch_gptimer_def* tdef);
	tch_pwm_handle* (*openAsPWM)(tch_timer timer,tch_pwm_def* tdef);
	tch_tcapt_handle* (*openTimerCapture)(tch_timer timer,tch_tcapt_def* tdef);
	void (*close)(tch_timer timer);
}tch_lld_timer;

extern const tch_lld_timer* tch_timer_instance;

#endif /* TCH_TIMER_H_ */
