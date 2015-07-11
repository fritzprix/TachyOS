/*
 * tch_segment.h
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 */

#ifndef TCH_SEGMENT_H_
#define TCH_SEGMENT_H_

#include "tch_mm.h"

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




struct mem_segment {
	rb_treeNode_t*			reg_root;
	rb_treeNode_t			addr_rbn;
	rb_treeNode_t			id_rbn;
	uint16_t				flags;
	uint32_t				poff;
	size_t					psize;			// total segment size in page
	cdsl_dlistNode_t 		pfree_list;		// free page list
	size_t	 				pfree_cnt;		// the total number of free pages in this segment
};

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

/**
 *  \brief register memory section into segment
 *  \param[in] pointer to input section desciptor
 *  \param[in] pointer to segment segment control block
 */
extern int tch_segmentRegister(struct section_descriptor* section,struct mem_segment* segment);
extern struct mem_segment* tch_segmentLookup(int seg_id);
extern void tch_segmentUnregister(int seg_id);
extern size_t tch_segmentGetSize(int seg_id);
extern size_t tch_segmentGetFreeSize(int seg_id);
extern void tch_mapSegment(struct tch_mm* mm,int seg_id);
extern void tch_unmapSegment(struct tch_mm* mm,int seg_id);


extern uint32_t tch_segmentAllocRegion(int seg_id,struct mem_region* mreg,size_t sz,uint16_t permission);
extern void tch_segmentFreeRegion(const struct mem_region* mreg);
extern struct mem_region* tch_segmentGetRegionFromPtr(void* ptr);
extern int tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg);
extern int tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg);




#endif /* TCH_SEGMENT_H_ */
