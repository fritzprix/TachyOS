/*
 * tch_segment.c
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 *
 *  \defgroup Segment Kernel
 */

#include "tch_kernel.h"
#include "tch_err.h"
#include "tch_mm.h"
#include "tch_segment.h"
#include "tch_kmalloc.h"

#define ptr_to_pgidx(ptr)		(((size_t) ptr) >> PAGE_OFFSET)


/**
 * when region is freed first page of region has header for region information
 */
struct free_page_header {
	cdsl_dlistNode_t 	lhead;
	uint32_t 			contig_pcount;
	uint32_t 			offset;
};


struct page_frame {
	union {
		uint8_t __dummy[PAGE_SIZE];
		struct free_page_header fhdr;
	};
};


/**
 *  statically declared memory management struct
 *  used to memory management bootstrap
 *
 */

static struct mem_segment init_seg;				// segment to be used as dynamic memory pool
static struct mem_region  init_dynamic_region;	// initial memory region, **NOTE : the only region which is statically declared
static int init_segid;

static rb_treeNode_t*		id_root;
static rb_treeNode_t*		addr_root;
static uint32_t				seg_cnt;
static struct proc_dynamic  init_dynamic;

static int initSegment(struct section_descriptor* section,struct mem_segment* seg);
static void initRegion(struct mem_region* regp,struct mem_segment* parent,uint32_t poff,uint32_t psz,uint32_t perm);
static struct mem_region* findRegionFromPtr(struct mem_segment* segp,void* ptr);
static struct mem_segment* findSegmentFromPtr(void* ptr);




void tch_initSegment(struct section_descriptor* init_section){
	mset(&init_seg,0,sizeof(struct mem_segment));
	seg_cnt = 0;

	init_segid = initSegment(init_section,&init_seg);
	init_mm.dynamic = &init_dynamic;
	init_mm.dynamic->mregions = NULL;
	init_mm.pgd = NULL;				// kernel has no mapping table in mpu based hardware
	tch_mapRegion(&init_mm,&init_dynamic_region);

	tch_kmalloc_init(init_segid);
	tch_shmInit(init_segid);
	init_mm.pgd = tch_port_allocPageDirectory(kmalloc);
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


int tch_segmentRegister(struct section_descriptor* section){
	if(!section)
		return -1;
	struct mem_segment *segment = kmalloc(sizeof(struct mem_segment));
	if(!segment)
		return -1;			// segment can't be registred any more
	return initSegment(section,segment);
}

void tch_segmentUnregister(int seg_id){
	if(seg_id < 0)
		return;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeDelete(&id_root,seg_id);		// delete segment from id rbtree
	if(!segment)
		return;
	segment = container_of(segment,struct mem_segment,id_rbn);
	if(segment == &init_seg)
		KERNEL_PANIC("kernel init segment can't be unregistered");

	cdsl_rbtreeDelete(&addr_root,segment->poff);												// delete segment from addr rbtree
	struct mem_region* reg;
	while(!segment->reg_root){
		reg = (struct mem_region*) cdsl_rbtreeDelete(&segment->reg_root,segment->reg_root->key);
		if(reg){
			reg = container_of(reg,struct mem_region,rbn);
			if(!reg && (reg->segp->id_rbn.key == seg_id))
				tch_segmentFreeRegion(reg);
		}
		kfree(reg);		// free mem_region struct
	}
	kfree(segment);		// free mem_segment
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

/**
 * \brief associate whole segment to specific thread
 */
void tch_mapSegment(struct tch_mm* mm,int seg_id){
	if(!mm || (seg_id < 0))
		return;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return;
	segment = container_of(segment,struct mem_segment,id_rbn);

	struct mem_region* region = kmalloc(sizeof(struct mem_region));
	if(!region)
		KERNEL_PANIC("mem_region can't created");
	initRegion(region,segment,segment->poff,segment->psize,get_permission(segment->flags));		// region has same page offset and count to its parent segment
	region->owner = mm;
	cdsl_rbtreeInsert(&mm->dynamic->mregions,&region->mm_rbn);

}

/**
 * \brief unmap segment from calling thread
 *  unmap segment which has been mapped to given mm node
 * \param[in]
 */
void tch_unmapSegment(struct tch_mm* mm,int seg_id){
	if(!mm || (seg_id < 0))
		return;
	struct mem_segment* segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id);
	if(!segment)
		return;
	segment = container_of(segment,struct mem_segment,id_rbn);
	if(cdsl_rbtreeIsNIL(segment->reg_root))
		return;

	struct mem_region* region = container_of(segment->reg_root,struct mem_region,rbn);
	region->owner = NULL;

	cdsl_rbtreeDelete(&mm->dynamic->mregions,region->poff);				// region is removed from all the tracking red black tree
	cdsl_rbtreeDelete(&segment->reg_root,region->poff);

	kfree(region);
}


/**
 *	\brief allocation region
 *	 allocate contiguous memory region from segment (Normal).
 *	\param[in] seg_id id of segment from which region is allocated
 *	\param[in] mreg memory region struct which allocation information is written to
 *	\param[in] access permission to the region
 *	\return if region is allocated successfully,size of region in page size will be returned, otherwise 0.
 */
uint32_t tch_segmentAllocRegion(int seg_id,struct mem_region* mreg,size_t sz,uint32_t permission){
	if(seg_id < 0)
		return 0;
	if((mreg == NULL) || (sz == 0)){
		mreg->poff = 0;
		mreg->psz = 0;
		return 0;
	}
	struct mem_segment* segment;
	segment = (struct mem_segment*) cdsl_rbtreeLookup(&id_root,seg_id); 		// find segment by id
	if(!segment)
		return 0;
	segment = container_of(segment,struct mem_segment,id_rbn);					// get actual segment struct pointer
	uint32_t pcount = sz / PAGE_SIZE;
	if(sz % PAGE_SIZE)
		pcount++;
	if(segment->pfree_cnt < pcount)
		return 0;

	struct page_frame* frame = (struct page_frame*) (container_of(segment->pfree_list.next,struct free_page_header,lhead));
	struct page_frame *nframe;
	while(frame){
		if(frame->fhdr.contig_pcount >= pcount){ 										// find contiguos page region
			initRegion(mreg, segment, frame->fhdr.offset, pcount,permission);
			segment->pfree_cnt -= mreg->psz;

			if((mreg->poff + mreg->psz) == (segment->poff + segment->psize)){			// if the segment is empty, no frame header added
				cdsl_dlistRemove(&frame->fhdr.lhead);
				return pcount;
			}
																						// otherwise, add new frame header after allocated region
			nframe = &frame[pcount];
			nframe->fhdr.contig_pcount = frame->fhdr.contig_pcount - pcount;			// set new contiguos free region
			cdsl_dlistReplace(&frame->fhdr.lhead,&nframe->fhdr.lhead);
			return pcount;
		}
		frame = (struct page_frame*) container_of(frame->fhdr.lhead.next,struct free_page_header,lhead);		// move to next frame
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

	if(cdsl_rbtreeDelete(&segment->reg_root,mreg->poff) != &mreg->rbn)										// delete memory region structure from allocation tree
		KERNEL_PANIC("region mapping broken");

	cdsl_dlistNode_t* phead = segment->pfree_list.next;
	page_frame_t* frame = (page_frame_t*) (mreg->poff << PAGE_OFFSET); /*15.09.18 : poff shift */
	page_frame_t* cframe;
	frame->fhdr.offset = mreg->poff;
	frame->fhdr.contig_pcount = mreg->psz;

	/**
	 * find position on the list where freed page region inserted
	 */
	do {
		cframe = (struct page_frame*) container_of(phead,struct free_page_header,lhead);
		phead = phead->next;
	} while((cframe->fhdr.offset < frame->fhdr.offset));


	cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.prev,struct free_page_header,lhead);		// step backward
	cdsl_dlistInsertAfter(&cframe->fhdr.lhead,&frame->fhdr.lhead);											// insert freed page region after found frame
	segment->pfree_cnt += mreg->psz;																		// update segment free size



	/**
	 *  begining of merge operation of region
	 */
	if(cframe == (struct page_frame*) &segment->pfree_list)
		cframe = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct free_page_header,lhead);

	/**
	 * find mergable region
	 */
	while(cframe->fhdr.contig_pcount + cframe->fhdr.offset == ((struct free_page_header*) container_of(cframe->fhdr.lhead.next,struct free_page_header,lhead))->offset){
		frame = (struct page_frame*) container_of(cframe->fhdr.lhead.next,struct free_page_header,lhead);
		cframe->fhdr.contig_pcount += frame->fhdr.contig_pcount;		// merge into bigger chunk
		cdsl_dlistRemove(&frame->fhdr.lhead);
	}
}

static int initSegment(struct section_descriptor* section,struct mem_segment* seg){
	if(!section || !seg)
		return -1;

	uint32_t i;
	seg->poff = ((size_t) section->start + PAGE_SIZE - 1) >> PAGE_OFFSET;				// calculate page index of segment's begining from section base address
	size_t section_limit = ((size_t) section->end)  >> PAGE_OFFSET; 					// calculate page index of segment's ending from sectioni size
	seg->psize = section_limit - seg->poff;													// segment size in pages
	page_frame_t* pages = (page_frame_t*) (seg->poff << PAGE_OFFSET);

	seg->flags = section->flags;															// inherit permission of section
	seg->reg_root = NULL;
	cdsl_dlistInit(&seg->pfree_list);														// init members
	cdsl_rbtreeNodeInit(&seg->id_rbn,seg_cnt);
	cdsl_rbtreeNodeInit(&seg->addr_rbn,seg->poff);


	switch(seg->flags & SEGMENT_MSK){
	case SEGMENT_KERNEL:
		seg->pfree_cnt = 0;											// marked as segment has no free page for kernel section & keep its memory content
		uint32_t sec_type = get_section(seg->flags);

		if(sec_type == SECTION_URODATA || sec_type == SECTION_UTEXT){
			set_permission(seg->flags,PERM_OTHER_RD | PERM_KERNEL_XC | PERM_KERNEL_ALL);
		} else {
			set_permission(seg->flags,PERM_KERNEL_ALL);					// kernel is only accessible privilidge level
		}
		break;
	case SEGMENT_NORMAL:
		seg->pfree_cnt = seg->psize;								// set free page count as its total page count
		for(i = 0; i < seg->psize ;i++){							// initialize all the page frame header structure within a segment
			pages[i].fhdr.offset = seg->poff + i;
			pages[i].fhdr.contig_pcount = 0;
			cdsl_dlistInit(&pages[i].fhdr.lhead);
		}

		pages[0].fhdr.contig_pcount = seg->psize;					// initialize first page frame header
		cdsl_dlistPutHead(&seg->pfree_list,&pages[0].fhdr.lhead);
		set_permission(seg->flags,PERM_KERNEL_ALL | PERM_OTHER_RD);	// normal default permission (kernel_all | public read)
		break;
	case SEGMENT_DEVICE:
		// TODO : make decision whether implement device segment or leave it to be accessed by priviledged context
		break;
	}

	cdsl_rbtreeInsert(&id_root,&seg->id_rbn);			// any registered segment doesn't need to be unregistered in typical case
	cdsl_rbtreeInsert(&addr_root,&seg->addr_rbn);		// tree is used only for lookup
	return seg_cnt++;
}

/**
 *  @brief initialize region struct and insert it to parent segment
 *  @param[in] regp	pointer to mem_region struct
 */
static void initRegion(struct mem_region* regp,struct mem_segment* parent,uint32_t poff,uint32_t psz,uint32_t perm){
	if(!regp)
		return;
	regp->poff = poff;
	regp->psz = psz;
	regp->segp = parent;
	regp->flags = parent->flags;
	regp->owner = NULL;
	if(perm)															// if new permission is available, override parent permission
		set_permission(regp->flags,perm);
	else
		set_permission(regp->flags,get_permission(parent->flags));		// otherwise,inherit permission from its parent
	cdsl_rbtreeNodeInit(&regp->rbn,regp->poff);
	cdsl_rbtreeNodeInit(&regp->mm_rbn,regp->poff);
	cdsl_rbtreeInsert(&parent->reg_root,&regp->rbn);					// add region tracking red-black tree
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
	if(!arb)
		KERNEL_PANIC("Segment has no allocated region -> Invalid Use of Pointer");
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
	if(!arb)
		KERNEL_PANIC("No Registered Segment in kernel -> Memory is not initialize properly");
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


void tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg){
	if(!mm || !mreg)
		return;
	mreg->owner = mm;
	cdsl_rbtreeInsert(&mm->dynamic->mregions,&mreg->mm_rbn);
}

void tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg){
	if(!mm || !mreg)
		return;
	mreg->owner = NULL;
	cdsl_rbtreeDelete(&mm->dynamic->mregions,mreg->poff);
}


int tch_regionGetSize(struct mem_region* mreg){
	if(!mreg)
		return 0;
	return (mreg->psz << PAGE_OFFSET);
}

