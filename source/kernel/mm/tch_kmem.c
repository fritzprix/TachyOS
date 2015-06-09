/*
 * tch_kmem.c
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */



#include "tch_kernel.h"
#include "tch_ktypes.h"
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
	uint32_t			magic;
};

struct page_frame {
	union {
		uint8_t __dummy[CONFIG_PAGE_SIZE];
		struct page_free_header __pf_hdr;
	};
};



static struct page_free_header __dummy_page_header;
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
	cdsl_dlistInit(&__dummy_page_header.lhead);
	__dummy_page_header.contig_pcount = 0;
	__dummy_page_header.offset = DUMMY_OFFSET;

	uint16_t i = 0;
	for(; i < PAGE_COUNT ; i++){
		kernel_dynamic[i].__pf_hdr.offset = i;
		kernel_dynamic[i].__pf_hdr.contig_pcount = 0;
		cdsl_dlistInit(&kernel_dynamic[i].__pf_hdr.lhead);
	}

	kernel_dynamic[0].__pf_hdr.contig_pcount = PAGE_COUNT;					// set whole dynamic memory as contiguos region
	cdsl_dlistPutHead(&free_page_list,&kernel_dynamic[0].__pf_hdr.lhead);
	cdsl_dlistPutTail(&free_page_list,&__dummy_page_header.lhead);

	return kernel_init_stack;
}

int tch_kernelStackSanity(){
	return *((uint32_t*)&__kernel_stack[0]) == KERNEL_STACK_OVFCHECK_MAGIC;
}


uint32_t tch_memAllocRegion(struct mem_region* mreg,size_t sz){
	uint16_t pcount = sz / CONFIG_PAGE_SIZE + 1;
	cdsl_dlistNode_t* phead = free_page_list.next;
	struct page_frame *cframe,*nframe;
	while(phead !=  (cdsl_dlistNode_t*) &__dummy_page_header){
		cframe = (struct page_frame*) phead;
		if(cframe->__pf_hdr.contig_pcount >= pcount){ // find contiguos page region
			mreg->p_offset = cframe->__pf_hdr.offset;
			mreg->p_cnt = pcount;
			nframe = &cframe[pcount];
			nframe->__pf_hdr.contig_pcount = cframe->__pf_hdr.contig_pcount - pcount;	// set new contiguos free region
			cdsl_dlistReplace(&cframe->__pf_hdr.lhead,&nframe->__pf_hdr.lhead);
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
	rframe->__pf_hdr.offset = mreg->p_offset;
	rframe->__pf_hdr.contig_pcount = mreg->p_cnt;
	if(((struct page_frame*) container_of(free_page_list.next,struct page_free_header,lhead))->__pf_hdr.offset > rframe->__pf_hdr.offset){
		cdsl_dlistPutHead(&free_page_list,rframe);
		cframe = (struct page_frame*) container_of(rframe->__pf_hdr.lhead.next,struct page_free_header,lhead);
		if(rframe->__pf_hdr.contig_pcount + rframe->__pf_hdr.offset == cframe->__pf_hdr.offset){
			rframe->__pf_hdr.contig_pcount += cframe->__pf_hdr.contig_pcount;	// merge first page region as contiguous region of freed page region
			cdsl_dlistRemove(&cframe->__pf_hdr.lhead);	// remove first node from page region list
		}
		return;
	}
	// if page offset of freed region is larger than first node of free region list
	while((phead != (cdsl_dlistNode_t*) &__dummy_page_header) && (cframe->__pf_hdr.offset < rframe->__pf_hdr.offset)){
		cframe = (struct page_frame*) container_of(phead,struct page_free_header,lhead);
		phead = phead->next;
	}
	cframe = (struct page_frame*) container_of(phead->prev,struct page_free_header,lhead);
	cdsl_dlistInsertAfter(&cframe->__pf_hdr.lhead,&rframe->__pf_hdr.lhead);
	while(cframe->__pf_hdr.contig_pcount + cframe->__pf_hdr.offset == rframe->__pf_hdr.offset){
		cframe->__pf_hdr.contig_pcount += rframe->__pf_hdr.contig_pcount;		// merge into bigger chunk
		cdsl_dlistRemove(&rframe->__pf_hdr.lhead);
		rframe = (struct page_frame*) container_of(cframe->__pf_hdr.lhead.next,struct page_free_header,lhead);
	}
}
