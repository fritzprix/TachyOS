/*
 * tch_segment.c
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_err.h"
#include "tch_mm.h"
#include "tch_segment.h"
#include "tch_kmalloc.h"



#define ptr_to_pgidx(ptr)		(((size_t) ptr) >> CONFIG_PAGE_SHIFT)


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
		uint8_t __dummy[PAGE_SIZE];
		struct page_free_header fhdr;
	};
};



static struct mem_segment init_seg;		// segment to be used as dynamic free memory pool
static struct mem_region init_dynamic_region;	// initial memory region
static int init_segid;

static rb_treeNode_t*		id_root;
static rb_treeNode_t*		addr_root;
static uint32_t				seg_cnt;

static struct mem_region* findRegionFromPtr(struct mem_segment* segp,void* ptr);
static struct mem_segment* findSegmentFromPtr(void* ptr);


void tch_initSegment(struct section_descriptor* init_section){
	memset(&init_seg,0,sizeof(struct mem_segment));
	init_segid = tch_segmentRegister(init_section,&init_seg);
	seg_cnt = 0;

	if(!(tch_segmentAllocRegion(init_segid, &init_dynamic_region, CONFIG_KERNEL_DYNAMICSIZE,PERM_KERNEL_ALL) > 0))
		KERNEL_PANIC("tch_segment.c","Can't create memory regions for kernel heap");

	init_mm.mregions = NULL;
	init_mm.pgd = NULL;				// kernel has no mapping table in mpu based hardware
	tch_mapRegion(&init_mm,&init_dynamic_region);

	tch_initKmalloc(init_segid);

}



int tch_segmentRegister(struct section_descriptor* section,struct mem_segment* seg){
	if(!section || !seg)
		return -1;

	uint32_t i;
	seg->poff = ((size_t) section->paddr + PAGE_SIZE - 1) >> CONFIG_PAGE_SHIFT;
	size_t section_limit = ((size_t) section->paddr + section->size)  >> CONFIG_PAGE_SHIFT;
	seg->psize = section_limit - seg->poff;			//page count
	page_frame_t* pages = (page_frame_t*) (seg->poff << CONFIG_PAGE_SHIFT);

	seg->flags = section->flags;
	cdsl_dlistInit(&seg->pfree_list);
	cdsl_rbtreeNodeInit(&seg->id_rbn,seg_cnt);
	cdsl_rbtreeNodeInit(&seg->addr_rbn,seg->poff);
	switch(seg->flags & SECTION_MSK){
	case SECTION_KERNEL:
		seg->pfree_cnt = 0;				// marked as segment has no free page for kernel section & keep its memory content
		break;
	case SECTION_NORMAL:
		seg->pfree_cnt = seg->psize;
		for(i = 0; i < seg->psize ;i++){
			pages[i].fhdr.offset = i;
			pages[i].fhdr.contig_pcount = 0;
			cdsl_dlistInit(&pages[i].fhdr.lhead);
		}
		pages[0].fhdr.contig_pcount = seg->psize;
		cdsl_dlistPutHead(&seg->pfree_list,&pages[0].fhdr.lhead);
		break;
	}

	cdsl_rbtreeInsert(&id_root,&seg->id_rbn);			// any registered segment doesn't need to be unregistered in typical case
	cdsl_rbtreeInsert(&addr_root,&seg->addr_rbn);		// tree is used only for lookup
	return seg_cnt++;
}

struct mem_segment* tch_segmentLookup(int seg_id){
	if(seg_id < 0)
		return NULL;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return NULL;
	segment = container_of(segment,struct mem_segment,id_rbn);
	return segment;
}

void tch_segmentUnregister(int seg_id){
	if(seg_id < 0)
		return;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeDelete(&id_root,seg_id);
	if(!segment)
		return;
	segment = container_of(segment,struct mem_segment,id_rbn);
	struct mem_region* reg;
	while(!segment->reg_root){
		reg = (struct mem_region*) cdsl_rbtreeDelete(&segment->reg_root,segment->reg_root->key);
		if(reg){
			reg = container_of(reg,struct mem_region,rbn);
			if(!reg && (reg->segp->id_rbn.key == seg_id))
				tch_segmentFreeRegion(reg);
		}
	}
}


size_t tch_segmentGetSize(int seg_id){
	if(seg_id < 0)
		return 0;
	struct mem_segment* segment = (struct mem_segment*)cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return 0;
	segment = container_of(segment,struct mem_segment,id_rbn);
	return segment->psize;
}

size_t tch_segmentGetFreeSize(int seg_id){
	if(seg_id < 0)
		return 0;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return 0;
	segment = container_of(segment,struct mem_segment,id_rbn);
	return segment->pfree_cnt;
}


void tch_mapSegment(struct tch_mm* mm,int seg_id){

}

void tch_unmapSegment(struct tch_mm* mm,int seg_id){

}



uint32_t tch_segmentAllocRegion(int seg_id,struct mem_region* mreg,size_t sz,uint16_t permission){
	if(seg_id < 0)
		return 0;
	if((mreg == NULL) || (sz == 0)){
		mreg->poff = 0;
		mreg->psz = 0;
		return 0;
	}
	struct mem_segment* segment;
	segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return 0;
	segment = container_of(segment,struct mem_segment,id_rbn);
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
			mreg->segp = segment;
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

void tch_segmentFreeRegion(const struct mem_region* mreg){
	if(mreg == NULL)
		return;
	if(mreg->psz == 0 ||!mreg->segp)
		return;
	struct mem_segment* segment = mreg->segp;
	if(!segment)
		return;
	if((mreg->poff >= segment->psize) && (mreg->psz > (segment->psize - mreg->poff)))			// may mregion is not valid
		return;

	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	page_frame_t* pages = (page_frame_t*) (segment->poff >> CONFIG_PAGE_SHIFT);
	struct page_frame* rframe,* cframe;
	rframe = &pages[mreg->poff];
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

struct mem_region* tch_segmentGetRegionFromPtr(void* ptr){
	if(!ptr)
		return NULL;
	struct mem_segment* segment = findSegmentFromPtr(ptr);
	return findRegionFromPtr(segment,ptr);
}


static struct mem_region* findRegionFromPtr(struct mem_segment* segp,void* ptr){
	if(!ptr || !segp)
		return NULL;

	rb_treeNode_t* arb = segp->reg_root;
	struct mem_region* region;
	uint32_t pgidx = ptr_to_pgidx(ptr);
	if(arb)
		KERNEL_PANIC("tch_segment.c","Segment has no allocated region -> Invalid Use of Pointer");
	while(!cdsl_rbtreeIsNIL(arb)){
		region = container_of(arb,struct mem_region,rbn);
		if((region->poff <= pgidx) && (region->poff + region->psz >= pgidx)){
			return region;
		}else if(region->poff > pgidx){
			arb = arb->left;
		}else if((region->poff + region->psz) < pgidx){
			arb = arb->right;
		}
	}
	return NULL;
}

static struct mem_segment* findSegmentFromPtr(void* ptr){
	if(!ptr)
		return NULL;
	rb_treeNode_t* arb = addr_root;
	struct mem_segment* segment;
	uint32_t pgidx = ptr_to_pgidx(ptr);
	if(arb)
		KERNEL_PANIC("tch_segment.c","No Registered Segment in kernel -> Memory is not initialize properly");
	while(!cdsl_rbtreeIsNIL(arb)){
		segment = container_of(arb,struct mem_segment,addr_rbn);
		if((segment->poff <= pgidx) && (segment->poff + segment->psize >= pgidx)){
			return segment;
		}else if(segment->poff > pgidx){
			arb = arb->left;
		}else if((segment->poff + segment->psize) < pgidx){
			arb = arb->right;
		}
	}
	return NULL;			// no matched segment
}


int tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg){

}

int tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg){

}


