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
#include "cdsl_rbtree.h"
#include "wtree.h"

#define ptr_to_pgidx(ptr)		(((size_t) ptr) >> PAGE_OFFSET)


/**
 * when region is freed first page of region has header for region information
 */
struct free_page_header {
	dlistNode_t         lhead;
	uint32_t            contig_pcount;
	uint32_t            offset;
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
static int                init_segid;

static rbtreeRoot_t	        id_root;           // global red black tree that keeps track of segment (key : seg_id)
static rbtreeRoot_t	        addr_root;         // global red black tree that keeps track of segment (key : page_offset)
static uint32_t	            seg_cnt;
static struct proc_dynamic  init_dynamic;

static int initSegment(struct section_descriptor* section,struct mem_segment* seg);
static struct mem_region* findRegionFromPtr(struct mem_segment* segp,void* ptr);
static struct mem_segment* findSegmentFromPtr(void* ptr);

static DECLARE_ONALLOCATE(seg_onallocate);

static wt_adapter seg_adapter = {
		.onallocate = seg_onallocate,
};


void tch_initSegment(struct section_descriptor* init_section){
	mset(&init_seg,0,sizeof(struct mem_segment));
	cdsl_rbtreeRootInit(&id_root);
	cdsl_rbtreeRootInit(&addr_root);
	seg_cnt = 0;

	init_segid = initSegment(init_section, &init_seg);
	init_mm.dynamic = &init_dynamic;
	cdsl_rbtreeRootInit(&init_mm.dynamic->mregions);
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
	while(cdsl_rbtreeIsEmpty(&segment->reg_root)){
		reg = (struct mem_region*) cdsl_rbtreeDelete(&segment->reg_root,segment->reg_root.entry->key);
		if(reg){
			reg = container_of(reg,struct mem_region,rbn);
			if(!reg && (reg->segp->id_rbn.key == seg_id))
				tch_segmentFreeRegion(reg, TRUE);
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
	tch_initRegion(region,segment,segment->poff, segment->psize, get_permission(segment->flags));		// region has same page offset and count to its parent segment
	region->owner = mm;
	cdsl_rbtreeInsert(&mm->dynamic->mregions, &region->mm_rbn, FALSE);

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
	if(cdsl_rbtreeIsEmpty(&segment->reg_root))
		return;

	struct mem_region* region = container_of(segment->reg_root.entry,struct mem_region,rbn);
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

	struct page_frame* frame = wtree_reclaim_chunk(&segment->alloc_root, (pcount << PAGE_OFFSET), FALSE);
	if(frame) {
		frame->fhdr.offset = ((size_t) frame) >> PAGE_OFFSET;
		tch_initRegion(mreg, segment, frame->fhdr.offset, pcount, permission);
		segment->pfree_cnt -= mreg->psz;
		return pcount;
	}
	return 0;
}

void tch_segmentFreeRegion(const struct mem_region* mreg, BOOL discard){
	if(mreg == NULL) {
		return;
	}
	if(mreg->psz == 0 ||!mreg->segp) {
		return;
	}
	struct mem_segment* segment = mreg->segp;
	if(!segment) {
		return;
	}
	if((mreg->poff >= segment->psize) && (mreg->psz > (segment->psize - mreg->poff))) {			// may mregion is not valid
		return;
	}

	if(cdsl_rbtreeDelete(&segment->reg_root,mreg->poff) != &mreg->rbn) {		                    // delete memory region structure from allocation tree
		KERNEL_PANIC("region mapping broken");
	}
	if(!discard) {
		return;
	}

	wtreeNode_t* reg_node = wtree_nodeInit(&segment->alloc_root, (uaddr_t) (mreg->poff << PAGE_OFFSET), mreg->psz << PAGE_OFFSET, NULL);
	if(!reg_node) {
		KERNEL_PANIC("INVALID REGION : fail to initialize Segment chunk");
	}
	wtree_addNode(&segment->alloc_root, reg_node, TRUE, NULL);
	segment->pfree_cnt += mreg->psz;																		// update segment free size
}



static int initSegment(struct section_descriptor* section,struct mem_segment* seg){
	if(!section || !seg)
		return -1;

	seg->flags = section->flags;                                                        // copy flags of section into segment
	seg->poff = ((size_t) section->start + PAGE_SIZE - 1) >> PAGE_OFFSET;               // calculate page index of segment's beginning from section base address
	size_t section_limit = ((size_t) section->end)  >> PAGE_OFFSET;                     // calculate page index of segment's ending from section size
	seg->psize = section_limit - seg->poff;                                             // segment size in pages

	if(section_limit < seg->poff) {
		// minimum size of section should be greater than single page
		// if the size is less than a page, section is discarded and return -1
		return -1;
	}

	cdsl_rbtreeRootInit(&seg->reg_root);
	cdsl_dlistEntryInit(&seg->pfree_list);
	cdsl_rbtreeNodeInit(&seg->id_rbn, seg_cnt);
	cdsl_rbtreeNodeInit(&seg->addr_rbn, seg->poff);


	uint32_t sec_type;
	switch(seg->flags & SEGMENT_MSK) {
	case SEGMENT_KERNEL:
		seg->pfree_cnt = 0;											// marked as segment has no free page for kernel section & keep its memory content
		sec_type = get_section(seg->flags);
		set_permission(seg->flags, PERM_KERNEL_ALL);            // kernel is only accessible privilege level
		break;
	case SEGMENT_NORMAL:
		seg->pfree_cnt = seg->psize;
		// create segment allocation root
		wtree_rootInit(&seg->alloc_root, NULL, &seg_adapter ,0);
		// register memory area of segment as available to dynamic memory allocation request
		wtreeNode_t* seg_node = wtree_baseNodeInit(&seg->alloc_root,(uaddr_t) (seg->poff << PAGE_OFFSET), seg->psize << PAGE_OFFSET);
		wtree_addNode(&seg->alloc_root, seg_node, FALSE, NULL);
		set_permission(seg->flags,PERM_KERNEL_ALL | PERM_OTHER_RD);	// normal default permission (kernel_all | public read)
		break;
	case SEGMENT_DEVICE:
		// TODO : make decision whether implement device segment or leave it to be accessed by privilege context
		break;
	case SEGMENT_UACCESS:
		seg->pfree_cnt = 0;
		sec_type = get_section(seg->flags);
		set_permission(seg->flags, PERM_OWNER_ALL | PERM_OTHER_RD | PERM_KERNEL_ALL);
		break;
	}

	cdsl_rbtreeInsert(&id_root,&seg->id_rbn, FALSE);               // any registered segment doesn't need to be unregistered in typical case
	cdsl_rbtreeInsert(&addr_root,&seg->addr_rbn, FALSE);           // tree is used only for lookup
	return seg_cnt++;
}

/**
 *  @brief initialize region struct and insert it to parent segment
 *  @param[in] regp	pointer to mem_region struct
 */
void tch_initRegion(struct mem_region* regp,struct mem_segment* parent,uint32_t poff,uint32_t psz,uint32_t perm){
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
	cdsl_rbtreeNodeInit(&regp->rbn, regp->poff);
	cdsl_rbtreeNodeInit(&regp->mm_rbn, regp->poff);
	cdsl_rbtreeInsert(&parent->reg_root, &regp->rbn, FALSE);            // add region tracking red-black tree
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

	rbtreeNode_t* arb = segp->reg_root.entry;
	struct mem_region* region;
	uint32_t pgidx = ptr_to_pgidx(ptr);
	if(!arb)
		KERNEL_PANIC("Segment has no allocated region -> Invalid Use of Pointer");
	while(!arb){
		region = container_of(arb, struct mem_region,rbn);
		if((region->poff <= pgidx) && (region->poff + region->psz >= pgidx)){
			return region;
		}else if(region->poff > pgidx){
			arb = cdsl_rbtreeGoLeft(arb);
		}else if((region->poff + region->psz) < pgidx){
			arb = cdsl_rbtreeGoRight(arb);
		}
	}
	return NULL;
}

static struct mem_segment* findSegmentFromPtr(void* ptr){
	if(!ptr)
		return NULL;
	rbtreeNode_t* arb = addr_root.entry;
	struct mem_segment* segment;
	uint32_t pgidx = ptr_to_pgidx(ptr);
	if(!arb)
		KERNEL_PANIC("No Registered Segment in kernel -> Memory is not initialize properly");
	while(!arb){
		segment = container_of(arb,struct mem_segment,addr_rbn);
		if((segment->poff <= pgidx) && (segment->poff + segment->psize >= pgidx)){
			return segment;
		}else if(segment->poff > pgidx){
			arb = cdsl_rbtreeGoLeft(arb);
		}else if((segment->poff + segment->psize) < pgidx){
			arb = cdsl_rbtreeGoRight(arb);
		}
	}
	return NULL;			// no matched segment
}


void tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg){
	if(!mm || !mreg)
		return;
	mreg->owner = mm;
	cdsl_rbtreeInsert(&mm->dynamic->mregions,&mreg->mm_rbn, FALSE);
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


static DECLARE_ONALLOCATE(seg_onallocate) {
	KERNEL_PANIC("Out-of-Memory\n");
	return NULL;
}


