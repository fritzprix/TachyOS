/*
 * tch_event.h
 *
 *  Created on: 2015. 1. 20.
 *      Author: innocentevil
 */

#ifndef TCH_EVENT_H_
#define TCH_EVENT_H_


#include "tch_kobj.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct tch_eventCb {
	tch_kobj            __obj;
	uint32_t              status;
	int32_t               ev_msk;
	int32_t               ev_signal;
	tch_thread_queue      ev_blockq;
} tch_eventCb;

extern tch_event_api_t Event_IX;
extern tchStatus tch_eventInit(tch_eventCb* evcb);
extern tchStatus tch_eventDeinit(tch_eventCb* evcb);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
