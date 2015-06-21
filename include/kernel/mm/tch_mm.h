/*
 * tch_mm.h
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */

#ifndef TCH_MM_H_
#define TCH_MM_H_

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



#ifdef __GNUC__
#define __kstack	__attribute__((section(".kernel.stack")))
#elif __IAR__
#endif


#ifdef __GNUC__
#define __kdynamic	__attribute__((section(".kernel.dynamic")))
#elif __IAR__
#endif


typedef struct page_frame page_frame_t;



struct segment_desciptor {
	void* 		p_addr;
	size_t		p_size;
	uint16_t	type;
#define SECTION_KIMAGE		// text / bss / data
#define SECTION_KSTACK		// stack for kernel
#define SECTION_DYNAMIC		// dynamic memory segment
#define SECTION_PERIMEM		// io memory segment
};


/**
 * represent dynamic memory pool
 */
struct mem_node {
	cdsl_dlistNode_t		lhead;
	cdsl_dlistNode_t 		page_freelist;
	page_frame_t* 			pages;
};

/**
 * represent allocated memory chunk from mem_node
 */
struct mem_region {
	rb_treeNode_t			rb_node;
	uint32_t				poffset;
	uint32_t				pcnt;
};


struct mm_struct {

};

extern uint32_t tch_memAllocRegion(struct mem_region* mreg,size_t sz);
extern void tch_memFreeRegion(const struct mem_region* mreg);

#endif /* TCH_MM_H_ */
