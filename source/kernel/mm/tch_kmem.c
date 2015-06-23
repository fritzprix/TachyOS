/*
 * tch_kmem.c
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */



#include "tch_kernel.h"
#include "tch_mm.h"

#define KERNEL_STACK_OVFCHECK_MAGIC			((uint32_t)0xFF00FF0)


#define DUMMY_OFFSET						((uint32_t) -1)
#define PAGE_COUNT							(CONFIG_KERNEL_DYNAMICSIZE / CONFIG_PAGE_SIZE)


/**
 * when region is freed first page of region has header for region information
 */
struct page_free_header {
	cdsl_dlistNode_t 	lhead;
	uint32_t 			contig_pcount;
	uint32_t 			offset;
};

struct page_frame {
	union {
		uint8_t __dummy[CONFIG_PAGE_SIZE];
		struct page_free_header fhdr;
	};
};


static struct page_free_header __dummy_page_header;
static uint8_t __kstack __kernel_stack[CONFIG_KERNEL_STACKSIZE];
static uint8_t __kdynamic __kernel_dynamic[CONFIG_KERNEL_DYNAMICSIZE];




static struct dynamic_segment kernel_mnode = {
		.pfree_list = {NULL,NULL},
		.pages = (struct page_frame*)__kernel_dynamic,
		.pfree_cnt = PAGE_COUNT
};



uint32_t* tch_kernelMemInit(){

	*((uint32_t*)&__kernel_stack[0]) = KERNEL_STACK_OVFCHECK_MAGIC;
	uint32_t* kernel_init_stack = (uint32_t*) __kernel_dynamic;
	kernel_init_stack--;
	int idx = 0;
	/**
	 * page map initialized
	 */
	cdsl_dlistInit(&kernel_mnode.pfree_list);
	cdsl_dlistInit(&__dummy_page_header.lhead);
	__dummy_page_header.contig_pcount = 0;
	__dummy_page_header.offset = DUMMY_OFFSET;

	uint16_t i = 0;
	for(; i < PAGE_COUNT ; i++){
		kernel_mnode.pages[i].fhdr.offset = i;
		kernel_mnode.pages[i].fhdr.contig_pcount = 0;
		cdsl_dlistInit(&kernel_mnode.pages[i].fhdr.lhead);
	}

	kernel_mnode.pages[0].fhdr.contig_pcount = PAGE_COUNT;					// set whole dynamic memory as contiguos region
	cdsl_dlistPutHead(&kernel_mnode.pfree_list,&kernel_mnode.pages[0].fhdr.lhead);
	cdsl_dlistPutTail(&kernel_mnode.pfree_list,&__dummy_page_header.lhead);

	return kernel_init_stack;
}

int tch_kernelStackSanity(){
	return *((uint32_t*)&__kernel_stack[0]) == KERNEL_STACK_OVFCHECK_MAGIC;
}


