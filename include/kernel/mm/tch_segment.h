/*
 * tch_segment.h
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 */

#ifndef TCH_SEGMENT_H_
#define TCH_SEGMENT_H_

#include "tch_ktypes.h"

#define PERM_KERNEL_RD		((uint16_t) 1)		// allows kernel process to read access		(all addresses are accessible from kernel in some implementation)
#define PERM_KERNEL_WR		((uint16_t) 2)		// allows kernel process to write access
#define PERM_KERNEL_XC		((uint16_t) 4)		// allows kernel process to execute access
#define PERM_KERNEL_ALL		(PERM_KERNEL_RD |\
							 PERM_KERNEL_WR |\
							 PERM_KERNEL_XC)	// allows kernel process to all access type

#define PERM_OWNER_RD		((uint16_t) 8)		// allows owner process to read access		(owner means process that initialy allocate the region)
#define PERM_OWNER_WR		((uint16_t) 16)		// allows owner process to write access
#define PERM_OWNER_XC		((uint16_t) 32)		// allows owner process to execute aceess

#define PERM_OTHER_RD		((uint16_t) 64)		// allows other process to read access
#define PERM_OTHER_WR		((uint16_t) 128)	// allows other process to write access
#define PERM_OTHER_XC		((uint16_t) 256)	// allows other process to execute access

#define tch_getRegionBase(regionp)			(void*) ((size_t) ((struct mem_region*)regionp)->poff * PAGE_SIZE)
#define tch_getRegionSize(regionp)			(size_t) (((struct mem_region*)regionp)->psz * PAGE_SIZE)



extern struct mem_segment;
/**
 * represent allocated memory chunk from mem_node
 */
struct mem_region {
	rb_treeNode_t			rbn;
	uint16_t				flags;
	rb_treeNode_t			rbnode;
	struct mem_segment*		segp;
	uint32_t				poff;
	uint32_t				psz;
};

extern void tch_initSegment(struct section_descriptor* init_section);

extern int tch_registerSegment(struct section_descriptor* section,struct mem_segment* segment);
extern void tch_unregisterSegment(int seg_id);

extern size_t tch_getSegmentSize(int seg_id);
extern size_t tch_getSegmentAvailable(int seg_id);

extern uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz,uint16_t permission);
extern void tch_freeRegion(const struct mem_region* mreg);


#endif /* TCH_SEGMENT_H_ */
