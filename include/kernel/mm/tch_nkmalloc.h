/*
 * tch_nkmalloc.h
 *
 *  Created on: 2016. 11. 5.
 *      Author: innocentevil
 */

#ifndef INCLUDE_KERNEL_MM_TCH_NKMALLOC_H_
#define INCLUDE_KERNEL_MM_TCH_NKMALLOC_H_

#include "tch_mm.h"
#include "tch_nsegment.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void init_km(int seg_id);
extern void *km_alloc(size_t sz);
extern tchStatus km_free(void* ptr);
extern void km_mstat(struct mstat_t* mstat);


#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_KERNEL_MM_TCH_NKMALLOC_H_ */
