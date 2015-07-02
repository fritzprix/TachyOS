/*
 * tch_nommu.c
 *
 *  Created on: Jun 28, 2015
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_mm.h"
#include "tch_phymm.h"


struct page_dummy{
	uint8_t 	__dummy[1 << CONFIG_PAGE_SHIFT];
} __attribute__((aligned(1 << CONFIG_PAGE_SHIFT)));

extern __kstack uint32_t __kernel_stack_start;

const struct memory_description km0_desc = {
		.flags = MEMTYPE_KERNEL,
		.address = NULL,
		.size = 0
};

const struct memory_description km1_desc = {
		.flags = MEMTYPE_NORMAL,
		.address = (&__kernel_stack_start - CONFIG_KERNEL_DYNAMICSIZE),
		.size = CONFIG_KERNEL_DYNAMICSIZE
};


const struct memory_description* __default_mem_desc[] = {
		&km0_desc,
		&km1_desc,
		NULL
};



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
 * 	   . kernel begins memory initialization from real mode(without enable paging unit)
 * 	   . kernel initialize large dynamic memory pool for whole system
 * 	   . initialize paging table (or other mapping equivalent)
 * 	   . kernel initialize its dedicated dynamic memory
 * 	   . kernel should prepare its own paging table before enable paging unit
 * 	   . kernel registers page entry of its footprint(loaded memory area) in kernel page table
 * 	3. initailization process by normal process
 * 	 - mpu based hardware
 * 	    . kernel create task structure (tch_thread_kheader) from kernel heap and initialize it.
 * 	    . kernel allocate pages (mem_region) where program is to be loaded(text / sdata / bss / stack).
 * 	    . kernel allocate pages for user process heap.
 * 	    . initialize task memory mapping table.
 *
 */




/**\brief kernel memory initailize
 *  - find stack memory
 *
 */
uint32_t* tch_kernelMemInit(struct memory_description** mdesc_tbl){

	int idx;
	struct memory_description* mem_desc = mdesc_tbl;
	if(!mem_desc)
		mem_desc = __default_mem_desc;

	while(!mem_desc){
		if(mem_desc->flags & MEMTYPE_KERNEL){
			// register memory access permission for kernel
		}else if(mem_desc->flags & MEMTYPE_NORMAL){

		}
	}


	if(!mdesc_tbl){

	}
}


void tch_registerPageMap(struct mem_region* mreg,const struct tch_pgmap* map,uint16_t permission){

}

void tch_unregisterPageMap(struct mem_region* mreg){

}

void tch_kernelOnMemFault(){

}
