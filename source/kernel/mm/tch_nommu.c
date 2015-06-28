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
} __attribute__((align(1 << CONFIG_PAGE_SHIFT)));

static __kstack uint8_t __kernel_stack_bottom;

static struct memory_description mem0_desc = {
		.flags = MEMTYPE_NORMAL,
		.address = (&__kernel_stack_bottom - CONFIG_KERNEL_DYNAMICSIZE),
		.size = CONFIG_KERNEL_DYNAMICSIZE
};


uint32_t* tch_kernelMemInit(struct memory_description* mdesc_tbl){

}

void* kalloc(size_t sz){

}

void kfree(void* p){

}

void tch_kernelOnMemFault(){

}
