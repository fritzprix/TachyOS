/*
 * tch_rendezvu.h
 *
 *  Created on: 2015. 12. 31.
 *      Author: doowoong lee
 */

#ifndef INCLUDE_KERNEL_TCH_RENDEZVU_H_
#define INCLUDE_KERNEL_TCH_RENDEZVU_H_

#include "kernel/tch_ktypes.h"
#include "kernel/tch_kobj.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_rendezv {
	tch_kobj				__kobj;
	uint32_t				status;
	tch_thread_queue 		wq;
}tch_rendzvCb;

extern tch_kernel_service_rendezvu Rendezvous_IX;
extern tchStatus tch_rendzvInit(tch_rendzvCb* rendvp);
extern tchStatus tch_rendzvDeinit(tch_rendzvCb* rendvp);

#if defined(__cplusplus)
}
#endif

#endif /* INCLUDE_KERNEL_TCH_RENDEZVU_H_ */
