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


typedef struct tch_eventCb tch_eventCb;

extern tch_eventId tchk_eventInit(tch_eventCb* evcb,tch_eventCb* initcb);
extern tchStatus tchk_eventWait(tch_eventId id,void* arg);
extern int32_t tchk_eventUpdate(tch_eventId id,void* arg);
extern void tchk_eventDeinit(tch_eventId id);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
