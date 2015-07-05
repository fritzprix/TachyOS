/*
 * tch_segment.h
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 */

#ifndef TCH_SEGMENT_H_
#define TCH_SEGMENT_H_

#include "tch_ktypes.h"

/**
 * represent allocated memory chunk from mem_node
 */
struct mem_region {
	rb_treeNode_t			rbn;
	uint16_t				perm;
	rb_treeNode_t			rbnode;
	struct mem_segment*		segp;
	uint32_t				poff;
	uint32_t				psz;
};

extern void tch_initSegment(struct section_descriptor* init_section);

extern int tch_registerSegment(struct section_descriptor* section);
extern void tch_unregisterSegment(int seg_id);

extern uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz);
extern void tch_freeRegion(const struct mem_region* mreg);


#endif /* TCH_SEGMENT_H_ */
