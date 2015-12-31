/*
 * tch_bar.h
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */

#ifndef TCH_BAR_H_
#define TCH_BAR_H_

#include "tch_kobj.h"
#include "tch_types.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_bar_cb {
	tch_kobj                 __obj;
	uint32_t                 status;
	uint8_t 				 th_cnt;
	cdsl_dlistNode_t         wq;
}tch_barCb;

extern tch_kernel_service_barrier Barrier_IX;
extern tchStatus tch_barInit(tch_barCb* bar,uint8_t th_cnt);
extern tchStatus tch_barDeinit(tch_barCb* bar);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BAR_H_ */
