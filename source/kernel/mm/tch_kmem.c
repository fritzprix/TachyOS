/*
 * tch_kmem.c
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */




#include "tch_ktypes.h"
#include "tch_mm.h"

#define KERNEL_STACK_OVFCHECK_MAGIC		((uint32_t)0xFF00FF0)





#define PAGE_COUNT	(CONFIG_KERNEL_DYNAMICSIZE / CONFIG_PAGE_SIZE)
struct page_free_header {
	cdsl_dlistNode_t 	lhead;
	uint32_t 	contig_pcount;
	uint16_t 	idx;
	uint32_t	magic;
};

struct page_frame {
	union {
		uint8_t __dummy[CONFIG_PAGE_SIZE];
		struct page_free_header __fr_hdr;
	};
};



static struct page_free_header __dummy_page_list;
static uint8_t __kstack __kernel_stack[CONFIG_KERNEL_STACKSIZE];
static uint8_t __kdynamic __kernel_dynamic[CONFIG_KERNEL_DYNAMICSIZE];


static cdsl_dlistNode_t free_page_list;
static struct page_frame* kernel_dynamic;



uint32_t* tch_kernelMemInit(){

	*((uint32_t*)&__kernel_stack[0]) = KERNEL_STACK_OVFCHECK_MAGIC;
	kernel_dynamic = (struct page_frame*) __kernel_dynamic;
	uint32_t* kernel_init_stack = (uint32_t*) __kernel_dynamic;
	kernel_init_stack--;
	int idx = 0;
	/**
	 * page map initialized
	 */
	cdsl_dlistInit(&free_page_list);
	cdsl_dlistInit(&__dummy_page_list.lhead);
	__dummy_page_list.contig_pcount = 0;

	uint16_t i = 0;
	for(; i < PAGE_COUNT ; i++){
		kernel_dynamic[i].__fr_hdr.idx = i;
		kernel_dynamic[i].__fr_hdr.contig_pcount = 0;
		cdsl_dlistInit(&kernel_dynamic[i].__fr_hdr.lhead);
	}

	kernel_dynamic[0].__fr_hdr.contig_pcount = PAGE_COUNT;					// set whole dynamic memory as contiguos region
	cdsl_dlistPutHead(&free_page_list,&kernel_dynamic[0].__fr_hdr.lhead);
	cdsl_dlistPutTail(&free_page_list,&__dummy_page_list.lhead);

	return kernel_init_stack;
}

int tch_kernelStackSanity(){
	return *((uint32_t*)&__kernel_stack[0]) == KERNEL_STACK_OVFCHECK_MAGIC;
}


uint32_t tch_memAllocRegion(struct mem_region* mreg,size_t sz){
	uint16_t pcount = sz / CONFIG_PAGE_SIZE + 1;
	cdsl_dlistNode_t* phead = free_page_list.next;
	struct page_frame *cframe,*nframe;
	while(phead !=  (cdsl_dlistNode_t*) &__dummy_page_list){
		cframe = (struct page_frame*) phead;
		if(cframe->__fr_hdr.contig_pcount >= pcount){ // find contiguos page region
			mreg->p_offset = cframe->__fr_hdr.idx;
			mreg->p_cnt = pcount;
			nframe = &cframe[pcount];
			nframe->__fr_hdr.contig_pcount = cframe->__fr_hdr.contig_pcount - pcount;	// set new contiguos free region
			cdsl_dlistReplace(&cframe->__fr_hdr.lhead,&nframe->__fr_hdr.lhead);
			return pcount;
		}
		phead = phead->next;
	}
	return 0;
}

void tch_memFreeRegion(const struct mem_region* mreg){
	if(!mreg)
		return;
	if((mreg->p_offset >= PAGE_COUNT) && (mreg->p_cnt > (PAGE_COUNT - mreg->p_offset)))			// may mregion is not valid
		return;

	cdsl_dlistNode_t* phead = free_page_list.next;
	struct page_frame* rframe,* cframe;
	rframe = &kernel_dynamic[mreg->p_offset];
	rframe->__fr_hdr.idx = mreg->p_offset;
	rframe->__fr_hdr.contig_pcount = mreg->p_cnt;
	while(phead != (cdsl_dlistNode_t*) &__dummy_page_list){
		cframe = (struct page_frame*) phead;
		if(cframe->__fr_hdr.idx < rframe->__fr_hdr.idx){			// if freed region and its adjacent region can be merged,
			if((cframe->__fr_hdr.idx + cframe->__fr_hdr.contig_pcount) == rframe->__fr_hdr.idx)
				cframe->__fr_hdr.contig_pcount += rframe->__fr_hdr.contig_pcount;
			else{

			}
		}
		phead = phead->next;
	}

}
