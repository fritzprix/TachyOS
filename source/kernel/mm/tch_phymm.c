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

#ifndef PAGE_MASK
#define PAGE_MASK				(PAGE_SIZE - 1)
#endif

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
		uint8_t __dummy[PAGE_SIZE];
		struct page_free_header fhdr;
	};
};


static struct dynamic_segroot segment_root = {NULL,0};

int tch_registerDynamicSegment(struct dynamic_segment* segment,void* base,size_t size){
	if(!segment)
		return -1;
	if(!segment->psize)
		return -1;

	segment->pages = (page_frame_t*) (((size_t) base + PAGE_SIZE) & ~PAGE_MASK);
	size_t seg_limit = ((size_t) base + size) & ~PAGE_MASK;
	segment->psize = seg_limit - (size_t) segment->pages;

	cdsl_dlistInit(&segment->pfree_list);


	cdsl_rbtreeNodeInit(&segment->rbnode,segment_root.seg_idx);
	cdsl_rbtreeInsert(&segment_root.root,&segment->rbnode);

	uint32_t i = 0;
	for(; i < segment->psize ;i++){
		segment->pages[i].fhdr.offset = i;
		segment->pages[i].fhdr.contig_pcount = 0;
		cdsl_dlistInit(&segment->pages[i].fhdr.lhead);
	}

	segment->pages[0].fhdr.contig_pcount = segment->psize;
	cdsl_dlistPutHead(&segment->pfree_list,&segment->pages[0].fhdr.lhead);

	cdsl_rbtreeNodeInit(&segment->rbnode,((size_t)base >> CONFIG_PAGE_SHIFT));
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
			reg = container_of(node,struct mem_region,lnode);
			if(!reg && (reg->segp->rbnode.key == seg_id))
				tch_memFreeRegion(reg);
		}
	}
	return segment;
}

uint32_t tch_allocRegion(int seg_id,struct mem_region* mreg,size_t sz){
	if(seg_id < 0)
		return 0;
	if((mreg == NULL) || (sz == 0)){
		mreg->poff = 0;
		mreg->psz = 0;
		return 0;
	}
	rb_treeNode_t* rbnode;
	rbnode = cdsl_rbtreeLookup(&segment_root.root,seg_id);
	if(!rbnode)
		return 0;
	struct dynamic_segment* segment = container_of(rbnode,struct dynamic_segment,rbnode);
	uint32_t pcount = sz / PAGE_SIZE;
	if(sz % PAGE_SIZE)
		pcount++;
	if(segment->pfree_cnt < pcount)
		return 0;

	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	struct page_frame *cframe,*nframe;
	while(phead !=  NULL){
		cframe = (struct page_frame*) phead;
		if(cframe->fhdr.contig_pcount >= pcount){ // find contiguos page region
			mreg->poff = cframe->fhdr.offset;
			mreg->psz = pcount;
			segment->pfree_cnt -= mreg->psz;
			if((mreg->poff + mreg->psz) == segment->psize){
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
	if(mreg->psz == 0 ||!mreg->segp)
		return;
	struct dynamic_segment* segment = container_of(mreg->segp,struct dynamic_segment,rbnode);
	if(!segment)
		return;
	if((mreg->poff >= segment->psize) && (mreg->psz > (segment->psize - mreg->poff)))			// may mregion is not valid
		return;


	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	struct page_frame* rframe,* cframe;
	rframe = &segment->pages[mreg->poff];
	rframe->fhdr.offset = mreg->poff;
	rframe->fhdr.contig_pcount = mreg->psz;
	// if page offset of freed region is larger than first node of free region list
	do {
		cframe = (struct page_frame*) container_of(phead,struct page_free_header,lhead);
		phead = phead->next;
	} while((cframe->fhdr.offset < rframe->fhdr.offset));

	cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.prev,struct page_free_header,lhead);
	cdsl_dlistInsertAfter(&cframe->fhdr.lhead,&rframe->fhdr.lhead);
	segment->pfree_cnt += mreg->psz;
	if(cframe == (struct page_frame*) &segment->pfree_list)
		cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);

	while(cframe->fhdr.contig_pcount + cframe->fhdr.offset == ((struct page_free_header*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead))->offset){
		rframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct page_free_header,lhead);
		cframe->fhdr.contig_pcount += rframe->fhdr.contig_pcount;		// merge into bigger chunk
		cdsl_dlistRemove(&rframe->fhdr.lhead);
	}
}


