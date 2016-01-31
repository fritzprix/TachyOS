/*
 * tch_waitq.h
 *
 *  Created on: 2016. 1. 31.
 *      Author: innocentevil
 */

#ifndef TCH_WAITQ_H_
#define TCH_WAITQ_H_


#include "kernel/tch_ktypes.h"
#include "kernel/tch_kobj.h"

#define DECLARE_GENERIC_COMPARE(fname)			void* 	__##fname__(void* a, void* b)
#if defined(__cplusplus)
extern "C" {
#endif


typedef struct tch_waitqcb {
	tch_kobj				__kobj;
	uint32_t				status;
	tch_thread_queue 		wq;
}tch_waitqCb;

extern tch_waitq_api_t WaitQ_IX;

extern tchStatus tch_waitqInit(tch_waitqCb* rendvp,tch_waitqPolicy policy);
extern tchStatus tch_waitqDeinit(tch_waitqCb* rendvp);

#if defined(__cplusplus)
}
#endif




#endif /* TCH_WAITQ_H_ */
