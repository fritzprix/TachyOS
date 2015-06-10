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
		struct page_free_header fhdr;
	};
};



static struct page_free_header __dummy_page_header;
static uint8_t __kstack __kernel_stack[CONFIG_KERNEL_STACKSIZE];
static uint8_t __kdynamic __kernel_dynamic[CONFIG_KERNEL_DYNAMICSIZE];


struct mem_node {
	cdsl_dlistNode_t	 	free_list;
	struct page_frame* 		pages;				// pointer to physical page frame table entry
	uint32_t 				alloc_fcnt;			// allocated frame count
};


static struct page_free_header __dummy_page_header;
static uint8_t __kstack __kernel_stack[CONFIG_KERNEL_STACKSIZE];
static uint8_t __kdynamic __kernel_dynamic[CONFIG_KERNEL_DYNAMICSIZE];


static struct mem_node kernel_mnode = {
		.pages = (struct page_frame*)__kernel_dynamic,
		.alloc_fcnt = PAGE_COUNT
};

struct page_frame 	pages;


uint32_t* tch_kernelMemInit(){

	*((uint32_t*)&__kernel_stack[0]) = KERNEL_STACK_OVFCHECK_MAGIC;
	uint32_t* kernel_init_stack = (uint32_t*) __kernel_dynamic;
	kernel_init_stack--;
	int idx = 0;
	/**
	 * page map initialized
	 */
	cdsl_dlistInit(&kernel_mnode.free_list);
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
	cdsl_dlistPutHead(&kernel_mnode.free_list,&kernel_mnode.pages[0].fhdr.lhead);
	cdsl_dlistPutTail(&kernel_mnode.free_list,&__dummy_page_header.lhead);

	return kernel_init_stack;
}

int tch_kernelStackSanity(){
	return *((uint32_t*)&__kernel_stack[0]) == KERNEL_STACK_OVFCHECK_MAGIC;
}


uint32_t tch_memAllocRegion(struct mem_region* mreg,size_t sz){
	if((mreg == NULL) || (sz == 0)){
		mreg->p_offset = 0;
		mreg->p_cnt = 0;
		return 0;
	}
	uint32_t pcount = sz / CONFIG_PAGE_SIZE;
	if(sz % CONFIG_PAGE_SIZE)
		pcount++;

	cdsl_dlistNode_t* phead = kernel_mnode.free_list.next;
	struct page_frame *cframe,*nframe;
	while(phead !=  (cdsl_dlistNode_t*) &__dummy_page_header){
		cframe = (struct page_frame*) phead;
		if(cframe->fhdr.contig_pcount >= pcount){ // find contiguos page region
			mreg->p_offset = cframe->fhdr.offset;
			mreg->p_cnt = pcount;
			kernel_mnode.alloc_fcnt -= mreg->p_cnt;
			if((mreg->p_offset + mreg->p_cnt) == PAGE_COUNT){
				cdsl_dlistRemove(&cframe->fhdr.lhead);
				return pcount;
			}
			nframe = &cframe[pcount];
			nframe->fhdr.contig_pcount = cframe->fhdr.contig_pcount - pcount;	// set new contiguos free region
			cdsl_dlistReplace(&cframe->fhdr.lhead,&nframe->fhdr.lhead);
			return pcount;
		}
		phead = phead->next;
	}
	return 0;
}

void tch_memFreeRegion(const struct mem_region* mreg){
	if(mreg == NULL)
		return;
	if((mreg->p_offset >= PAGE_COUNT) && (mreg->p_cnt > (PAGE_COUNT - mreg->p_offset)))			// may mregion is not valid
		return;
	if(mreg->p_cnt == 0)
		return;

	cdsl_dlistNode_t* phead = kernel_mnode.free_list.next;
	struct page_frame* rframe,* cframe;
	rframe = &kernel_mnode.pages[mreg->p_offset];
	rframe->fhdr.offset = mreg->p_offset;
	rframe->fhdr.contig_pcount = mreg->p_cnt;
	// if page offset of freed region is larger than first node of free region list
	do {
		cframe = (struct page_frame*) container_of(phead,struct page_free_header,lhead);
		phead = phead->next;
	} while((cframe->fhdr.offset < rframe->fhdr.offset));

	cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.prev,struct page_free_header,lhead);
	cdsl_dlistInsertAfter(&cframe->fhdr.lhead,&rframe->fhdr.lhead);
	kernel_mnode.alloc_fcnt += mreg->p_cnt;
	if(cframe == (struct page_frame*) &kernel_mnode.free_list)
		cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);

	while(cframe->fhdr.contig_pcount + cframe->fhdr.offset == ((struct page_free_header*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead))->offset){
		rframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);
		cframe->fhdr.contig_pcount += rframe->fhdr.contig_pcount;		// merge into bigger chunk
		cdsl_dlistRemove(&rframe->fhdr.lhead);
	}
}

uint32_t tch_memAvailable(){
	return kernel_mnode.alloc_fcnt;
}

uint32_t tch_memGetPageSize(){
	return CONFIG_PAGE_SIZE;
}
