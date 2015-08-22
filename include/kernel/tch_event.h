/*
 * tch_event.h
 *
 *  Created on: 2015. 1. 20.
 *      Author: innocentevil
 */

#ifndef TCH_EVENT_H_
#define TCH_EVENT_H_

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

extern tch_eventId tch_eventInit(tch_eventCb* evcb,BOOL is_static);
extern tchStatus tch_eventDeinit(tch_eventCb* evcb);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
