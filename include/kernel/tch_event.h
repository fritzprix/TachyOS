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

extern tchStatus tch_event_kwait(tch_eventId id,void* arg);
extern int32_t tch_event_kupdate(tch_eventId id,void* arg);
extern void tch_event_destroy(tch_eventId id);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
