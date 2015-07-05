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
#include "cdsl_rbtree.h"


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
#define CONFIG_KERNEL_DYNAMICSIZE 	(16 << 10)
#endif

#ifndef CONFIG_PAGE_SHIFT
#define CONFIG_PAGE_SHIFT			(10)
#endif

#define PAGE_SIZE		 			(1 << CONFIG_PAGE_SHIFT)
#define PAGE_MASK					(~(PAGE_SIZE - 1))


typedef void*	paddr_t;

struct section_descriptor {
	uint16_t		flags;
#define SECTION_NORMAL		((uint16_t) 0)
#define SECTION_KERNEL		((uint16_t) 1)		//	memory segment which is kernel is loaded
#define SECTION_MSK			(SECTION_KERNEL | SECTION_NORMAL)


#define TYPE_TEXT			((uint16_t) 4)
#define TYPE_SDATA			((uint16_t) 5)
#define TYPE_DATA			((uint16_t) 6)
#define TYPE_STACK			((uint16_t) 7)
#define TYPE_DYNAMIC		((uint16_t) 8)
#define TYPE_MSK			(~(TYPE_TEXT - 1))
	paddr_t* 		paddr;
	size_t			size;
}__attribute__((packed));



#ifdef __GNUC__
#define __kstack	__attribute__((section(".kernel.stack")))
#elif __IAR__
#endif

typedef struct page_frame page_frame_t;

struct tch_mm {
	rb_treeNode_t*			mregions;
	pgd_t*					pgd;
};

#define PERMISSION_WRITE	((perm_t) 1)
#define PERMISSION_READ		((perm_t) 2)
#define PERMISSION_EXC		((perm_t) 4)

extern struct tch_mm*	current_mm;

extern uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl);

extern int tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg);
extern int tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg);


#endif /* TCH_MM_H_ */
