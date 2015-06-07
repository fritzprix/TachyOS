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

struct mem_region {
	cdsl_dlistNode_t	lnode;
	tch_threadId 		owner;
	uint32_t			p_offset;
	uint16_t			p_cnt;
#define MAX_PAGECOUNT 		((uint16_t) 4)
};



extern uint32_t* tch_kernelMemInit();
extern int tch_kernelStackSanity();
extern uint32_t tch_memAllocRegion(struct mem_region* mreg,size_t sz);
extern void tch_memFreeRegion(const struct mem_region* mreg);

#endif /* TCH_MM_H_ */
