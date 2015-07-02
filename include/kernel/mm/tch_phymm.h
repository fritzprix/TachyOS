/*
 * tch_phymm.h
 *
 *  Created on: Jun 25, 2015
 *      Author: innocentevil
 */

#ifndef TCH_PHYMM_H_
#define TCH_PHYMM_H_


#include "tch_mm.h"
#include "tch_ktypes.h"




extern int tch_registerDynamicSegment(struct dynamic_segment* segment,void* base,size_t size);
extern struct dynamic_segment* tch_unregisterDynamicSegment(int seg_id);

extern uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz);
extern void tch_freeRegion(const struct mem_region* mreg);




#endif /* TCH_PHYMM_H_ */
