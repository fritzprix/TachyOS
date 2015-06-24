/*
 * tch_phym.c
 *
 *  Created on: 2015. 6. 23.
 *      Author: innocentevil
 */




/*
 * tch_kmem.c
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */



#include "tch_kernel.h"
#include "tch_phymm.h"




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

struct dynamic_segroot {
	rb_treeNode_t*		root;
	uint32_t			seg_cnt;
	int					seg_idx;
};

struct page_frame {
	union {
		uint8_t __dummy[CONFIG_PAGE_SIZE];
		struct page_free_header fhdr;
	};
};


static struct page_free_header __dummy_page_header;
static struct dynamic_segroot segment_root = {NULL,0};

int tch_registerDynamicSegment(struct dynamic_segment* segment){
	if(!segment)
		return -1;
	if(!segment->psize)
		return -1;
	cdsl_rbtreeNodeInit(&segment->rbnode,segment_root.seg_idx);
	cdsl_rbtreeInsert(&segment_root.root,&segment->rbnode);
	return segment_root.seg_idx++;
}

struct dynamic_segment* tch_unregisterDynamicSegment(int seg_id){
	if(seg_id < 0)
		return  NULL;
	struct dynamic_segment* segment = container_of(cdsl_rbtreeDelete(&segment_root.root,seg_id),struct dynamic_segment,rbnode);
	struct mem_region* reg;
	cdsl_dlistNode_t* node;
	while(!segment->reg_lhead.next){
		node = cdsl_dlistDequeue(&segment->reg_lhead);
		if(node){
			reg = container_of(node,struct mem_region,reg_lnode);
			if(!reg & (reg->seg_id == seg_id))
				tch_memFreeRegion(reg);
		}
	}
	return segment;
}

uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz){
	if(seg_id < 0)
		return 0;
	if((mreg == NULL) || (sz == 0)){
		mreg->p_offset = 0;
		mreg->p_cnt = 0;
		return 0;
	}
	rb_treeNode_t* rbnode;
	rbnode = cdsl_rbtreeLookup(&segment_root.root,seg_id);
	if(!rbnode)
		return 0;
	struct dynamic_segment* segment = container_of(rbnode,struct dynamic_segment,rbnode);
	uint32_t pcount = sz / CONFIG_PAGE_SIZE;
	if(sz % CONFIG_PAGE_SIZE)
		pcount++;
	if(segment->pfree_cnt < pcount)
		return 0;

	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	struct page_frame *cframe,*nframe;
	while(phead !=  (cdsl_dlistNode_t*) &__dummy_page_header){
		cframe = (struct page_frame*) phead;
		if(cframe->fhdr.contig_pcount >= pcount){ // find contiguos page region
			mreg->p_offset = cframe->fhdr.offset;
			mreg->p_cnt = pcount;
			segment->pfree_cnt -= mreg->p_cnt;
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

void tch_freeRegion(const struct mem_region* mreg){
	if(mreg == NULL)
		return;
	if((mreg->p_offset >= PAGE_COUNT) && (mreg->p_cnt > (PAGE_COUNT - mreg->p_offset)))			// may mregion is not valid
		return;
	if(mreg->p_cnt == 0 ||(mreg->seg_id < 0))
		return;

	struct dynamic_segment* segment = container_of(cdsl_rbtreeLookup(&segment_root.root,mreg->seg_id),struct dynamic_segment,rbnode);
	if(!segment)
		return;

	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	struct page_frame* rframe,* cframe;
	rframe = &segment->pages[mreg->p_offset];
	rframe->fhdr.offset = mreg->p_offset;
	rframe->fhdr.contig_pcount = mreg->p_cnt;
	// if page offset of freed region is larger than first node of free region list
	do {
		cframe = (struct page_frame*) container_of(phead,struct page_free_header,lhead);
		phead = phead->next;
	} while((cframe->fhdr.offset < rframe->fhdr.offset));

	cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.prev,struct page_free_header,lhead);
	cdsl_dlistInsertAfter(&cframe->fhdr.lhead,&rframe->fhdr.lhead);
	segment->pfree_cnt += mreg->p_cnt;
	if(cframe == (struct page_frame*) &segment->pfree_list)
		cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);

	while(cframe->fhdr.contig_pcount + cframe->fhdr.offset == ((struct page_free_header*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead))->offset){
		rframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);
		cframe->fhdr.contig_pcount += rframe->fhdr.contig_pcount;		// merge into bigger chunk
		cdsl_dlistRemove(&rframe->fhdr.lhead);
	}
}


