/*
 * tch_systimer.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_VTIMER_H_
#define TCH_VTIMER_H_

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
	osStatus (*start)(tch_timer_id timer,uint32_t millisec);
	osStatus (*stop)(tch_timer_id timer);
	osStatus (*delete)(tch_timer_id timer);
};


#endif /* TCH_SYSTIMER_H_ */
