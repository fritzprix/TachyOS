/*
 * tch_event.h
 *
 *  Created on: 2014. 11. 8.
 *      Author: innocentevil
 */

#ifndef TCH_EVENT_H_
#define TCH_EVENT_H_

#include "tch.h"
#include "tch_kernel.h"

#if defined(__cplusplus)
extern "C" {
#endif


typedef void (*tch_eventCallback)(int id,tch* env,void* arg);

struct tch_event_ix_t {
	tchStatus (*listen)(const tch* env,int id, tch_eventCallback cb,uint32_t timeout);
	tchStatus (*notify)(const tch* env,int id, void* ev_arg);
};


#if defined (__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
