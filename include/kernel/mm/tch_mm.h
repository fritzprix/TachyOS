/*
 * tch_mm.h
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */

#ifndef TCH_MM_H_
#define TCH_MM_H_

#include "tch_ktypes.h"
#include "tch_port.h"


/**
 * tachyos aims to allowing multiple program to run simultaneously without any interference between processes
 * or any harm to system stability. so there must be protection unit at the minumum in hardware. kernel itself dosen't has
 * any limitation (I believe) to support low cost 8-bit microcontrollers though, there are many alternatives which is more
 * appropriately designed to those kinds of low cost things.
 *
 *
 * mmu-less architecture (mpu supported)
 * 	. kernel operates in privilidged mode which is able to access all memory directly
 * mmu suppoted architecture
 * 	. kernel operates in privilidged mode though, it can not directly access all address
 * 	. kernel has to prepare its own page table before enabling paging hardware
 *
 * 	1. how memory is virtualized in each hardware architecture respectively.
 * 	- mpu based hardware (most of low(not extremely low....) cost system)
 * 	 . make decision whether accessed memory address is valid or not in memory fault (by cpu)
 * 	 . if it is valid address situation (mpu) update memory permission (protection) table in mpu configuration registers as soon as possible
 * 	   . if all the permission table entry is occupied (arm v7m mpu supports only 8 protection region..), drop least-likely to be used entry, and put new one.
 * 	- mmu based hardware (most of full featured processor like arm v7a or equivalent of.. which includes hardware paging unit )
 * 	 . make decision whether accessed memory address is valid or not (by mmu)
 * 	   .if it is valid mmu translate it to physical address and read or write data into memory (or cache)
 * 	   .if it is not valid address, mmu raises page fault when...
 * 	   	. page entry is valid but it's not present in main memory (swapped or not yet loaded from file)
 * 	   	. page entry is not valid, it's considered as error case, typically the task is signaled to terminate(SIGSEGV)
 *
 * 	2. initialization process by kernel
 * 	 - mpu based hardware
 * 	   . kernel has privilidged permission all the memory address space so initialization is simpler than mmu case
 * 	   . kernel initialize large dynamic memory pool for whole system
 * 	   . Dedicated dynamic memory for kernel is initialized (only privilidged access is permitted)
 * 	   . Shareable dynamic memory for communicating between user & kernel mode but is not writable from user (uni-directional) is initialized
 * 	   . set mpu register to enable
 * 	 - mmu based hardware
 * 	   .
 * 	3. initailization process by normal process
 * 	 - mpu based hardware
 * 	    . kernel create task structure (tch_thread_kheader) from kernel heap and initialize it.
 * 	    . kernel allocate pages (mem_region) where program is to be loaded(text / sdata / bss / stack).
 * 	    . kernel allocate pages for user process heap.
 * 	    . initialize task memory mapping table.
 *
 */


#ifndef CONFIG_KERNEL_STACKSIZE
#define CONFIG_KERNEL_STACKSIZE 	(4 << 10)
#endif

#ifndef CONFIG_KERNEL_DYNAMICSIZE
#define CONFIG_KERNEL_DYNAMICSIZE 	(32 << 10)
#endif

#ifndef CONFIG_PAGE_SHIFT
#define CONFIG_PAGE_SHIFT			(10)
#endif

#define PAGE_SIZE		 			(1 << CONFIG_PAGE_SHIFT)
#define PAGE_MASK					(~(PAGE_SIZE - 1))

struct memory_description {
	uint32_t		flags;
#define MEMTYPE_NORMAL		((uint32_t) 2)
#define MEMTYPE_KERNEL		((uint32_t) 1)		//	memory segment which is kernel is loaded
	void* 			address;
	size_t			size;
};



#ifdef __GNUC__
#define __kstack	__attribute__((section(".kernel.stack")))
#elif __IAR__
#endif

typedef struct page_frame page_frame_t;


struct tch_mm {
	void*			pgd;
};

struct tch_pgmap {
	uint32_t 		p_offset;			// physical page frame offset
	uint32_t		p_sz;				// physical memory region's contiguous size in pages
	uint32_t		v_offset;			// virtual memory region offset in pages
	uint32_t		v_sz;				// virtual memory region's contiguous size in pages
};
/**
 * represent allocated memory chunk from mem_node
 */
struct mem_region {
	cdsl_dlistNode_t		lnode;
	uint16_t				perm;
	rb_treeNode_t			rbnode;
	struct dynamic_segment* segp;
	uint32_t				poff;
	uint32_t				psz;
	uint32_t				voff;
	uint32_t				vsz;
};

/**
 * represent dynamic memory pool
 */
struct dynamic_segment {
	cdsl_dlistNode_t		reg_lhead;
	rb_treeNode_t			rbnode;
	struct mem_region 		pmap_reg;
	void*					pmap;
	page_frame_t* 			pages;			// physical page frames
	uint32_t				psize;			// total segment size in page
	cdsl_dlistNode_t 		pfree_list;		// free page list
	uint32_t 				pfree_cnt;		// the total number of free pages in this segment
};


#define PERMISSION_WRITE	((perm_t) 1)
#define PERMISSION_READ		((perm_t) 2)
#define PERMISSION_EXC		((perm_t) 4)

extern struct tch_mm*	current_mm;

extern uint32_t* tch_kernelMemInit(struct memory_description** mdesc_tbl);


extern void tch_registerPageMap(struct mem_region* mreg,const struct tch_pgmap* map,uint16_t permission);
extern void tch_unregisterPageMap(struct mem_region* mreg);



#endif /* TCH_MM_H_ */
