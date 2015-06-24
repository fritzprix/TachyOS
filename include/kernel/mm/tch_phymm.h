/*
 * tch_phymm.h
 *
 *  Created on: Jun 25, 2015
 *      Author: innocentevil
 */

#ifndef TCH_PHYMM_H_
#define TCH_PHYMM_H_


#include "tch_ktypes.h"

#ifndef CONFIG_KERNEL_STACKSIZE
#define CONFIG_KERNEL_STACKSIZE (4 << 10)
#endif

#ifndef CONFIG_KERNEL_DYNAMICSIZE
#define CONFIG_KERNEL_DYNAMICSIZE (32 << 10)
#endif


#ifndef CONFIG_PAGE_SIZE
#define CONFIG_PAGE_SIZE (1 << 10)
#endif


typedef struct page_frame page_frame_t;

/**
 * represent dynamic memory pool
 */
struct dynamic_segment {
	cdsl_dlistNode_t		reg_lhead;
	rb_treeNode_t			rbnode;
	page_frame_t* 			pages;			// physical page frames
	uint32_t				psize;			// total segment size in page
	cdsl_dlistNode_t 		pfree_list;		// free page list
	uint32_t 				pfree_cnt;		// the total number of free pages in this segment
};

/**
 * represent allocated memory chunk from mem_node
 */
struct mem_region {
	cdsl_dlistNode_t		reg_lnode;
	rb_treeNode_t			rb_node;
	int 					seg_id;
	uint32_t				p_offset;
	uint32_t				p_cnt;
};

extern int tch_registerDynamicSegment(struct dynamic_segment* segment);
extern struct dynamic_segment* tch_unregisterDynamicSegment(int seg_id);

extern uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz);
extern void tch_freeRegion(const struct mem_region* mreg);




#endif /* TCH_PHYMM_H_ */
