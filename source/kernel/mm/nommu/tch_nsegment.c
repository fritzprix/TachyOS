/*
 * tch_nsegment.c
 *
 *  Created on: 2016. 10. 29.
 *      Author: innocentevil
 */


#include "tch_nsegment.h"
#include "tch_mm.h"
#include "tch_err.h"

#include "cdsl_nrbtree.h"
#include "wtree.h"

static struct mem_nsegment init_segment;
static nrbtreeRoot_t seg_addr_root;
static nrbtreeRoot_t seg_id_root;
static wtreeRoot_t seg_pool;
static int seg_id;

static wt_adapter adapter = {0, };

static void init_segment_from_section(const struct section_descriptor* const section, struct mem_nsegment* segp, int seg_id);

int tch_init_segment(const struct section_descriptor* const init_section) {

	seg_id = 0;
	wtree_rootInit(&seg_pool, NULL, &adapter, 0);
	cdsl_nrbtreeRootInit(&seg_addr_root);
	cdsl_nrbtreeRootInit(&seg_id_root);

	init_segment_from_section(init_section, &init_segment, ++seg_id);

	wtreeNode_t* seg_node = wtree_baseNodeInit(&seg_pool, get_addr_from_page(init_segment.poff), get_addr_from_page(init_segment.psize));
	wtree_addNode(&seg_pool, seg_node, FALSE, NULL);

	return seg_id;
}

int tch_register_section(const struct section_descriptor* const section) {
	if(!section) {
		return -1;
	}
	struct mem_nsegment* nsegment = km_alloc(sizeof(struct mem_nsegment));
	if(!nsegment) {
		KERNEL_PANIC("fail to allocate new segment");
	}
	init_segment_from_section(section, nsegment, ++seg_id);
	wtreeNode_t* seg_node = wtree_baseNodeInit(&seg_pool, get_addr_from_page(nsegment->poff), get_size_from_pcount(nsegment->psize));
	wtree_addNode(&seg_pool, seg_node, FALSE, NULL);
	return seg_id;
}

tchStatus tch_allocate_region(struct mem_nregion* region, const size_t sz, uint32_t flags) {
	if(!region || !sz) {
		return tchErrorParameter;
	}
	int page_aligned_size = PAGE_SIZE;
	while(sz > page_aligned_size) {
		page_aligned_size += page_aligned_size;
	}
	void* reg_chunk = wtree_reclaim_chunk(&seg_pool, page_aligned_size, TRUE);
	if(!reg_chunk) {
		return tchErrorNoMemory;
	}
	if((((size_t) reg_chunk >> PAGE_OFFSET) << PAGE_OFFSET) != ((size_t) reg_chunk)) {
		KERNEL_PANIC("unaligned region");
	}
	region->flags = flags;
	region->poff = get_page_from_addr((size_t) reg_chunk);
	region->psize = get_pcount_from_size((size_t) page_aligned_size);
	cdsl_nrbtreeNodeInit(&region->map_rbn, region->poff);
	return tchOK;
}

tchStatus tch_free_region(const struct mem_nregion* region) {
	if(!region) {
		return tchErrorParameter;
	}
	if(region->owner) {
		tch_unmap_region(region);
	}

	wtreeNode_t* node = wtree_nodeInit(&seg_pool, get_addr_from_page(region->poff), get_size_from_pcount(region->psize), NULL);
	wtree_addNode(&seg_pool, node, TRUE, NULL);
	return tchOK;
}

// add region into
tchStatus tch_map_region(struct tch_mm* mm, struct mem_nregion* region) {
	if(!mm || !region) {
		return tchErrorParameter;
	}
	cdsl_nrbtreeInsert(&mm->dynamic->mregions, &region->map_rbn, FALSE);
	region->owner = mm;
	return tchOK;
}


tchStatus tch_unmap_region(struct mem_nregion* region) {
	if(!region || !region->owner) {
		return tchErrorParameter;
	}
	struct proc_dynamic* proc = region->owner->dynamic;
	if(cdsl_nrbtreeDelete(&proc->mregions, region->map_rbn.key)) {
		region->owner = NULL;       // set owner to null as a indication of unmapped region
		return tchOK;
	}
	return tchErrorParameter;
}

static void init_segment_from_section(const struct section_descriptor* const section, struct mem_nsegment* segp, int seg_id) {
	if(!section || !segp) {
		return;
	}

	segp->flags = section->flags;    // type 에 대한 속성을 그대로 가져온다.
	segp->poff = get_page_from_addr((section->start + PAGE_SIZE) & ~(PAGE_SIZE));
	segp->psize = get_page_from_addr(section->end - ((section->start + PAGE_SIZE) & ~(PAGE_SIZE)));

	cdsl_nrbtreeNodeInit(&segp->addr_rbn, segp->poff);
	cdsl_nrbtreeNodeInit(&segp->id_rbn, seg_id);
	cdsl_nrbtreeRootInit(&segp->reg_root);

	cdsl_nrbtreeInsert(&seg_addr_root, &segp->addr_rbn, FALSE);
	cdsl_nrbtreeInsert(&seg_id_root, &segp->id_rbn, FALSE);
}
