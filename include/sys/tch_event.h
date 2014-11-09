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

typedef void* tch_eventNode;
typedef void (*tch_eventCallback)(int id,tch_eventNode* node,void* arg);


struct tch_event_ix_t {
	tch_eventNode (*create)(const tch* env,const char* name);
	tchStatus (*listen)(tch_eventNode evnode,int id, tch_eventCallback cb,uint32_t timeout);
	tchStatus (*notify)(tch_eventNode evnode,int id, void* ev_arg);
	tchStatus (*destroy)(tch_eventNode evnode);
};


#if defined (__cplusplus)
}
#endif


#endif /* TCH_EVENT_H_ */
