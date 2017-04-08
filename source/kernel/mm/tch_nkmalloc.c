/*
 * tch_nkmalloc.c
 *
 *  Created on: 2016. 11. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_err.h"
#include "tch_nkmalloc.h"
#include "wtree.h"
#include "quantum.h"

#ifndef KM_BUDDY_MAX_ORDER
#define KM_BUDDY_MAX_ORDER             ((uint8_t) 6)
#endif

#ifndef KM_BUDDY_MIN_ORDER
#define KM_BUDDY_MIN_ORDER             ((uint8_t) 2)
#endif

#define KM_BUDDY_ORDER_COUNT           ((size_t) KM_BUDDY_MAX_ORDER - KM_BUDDY_MIN_ORDER)
#define KM_MIN_SIZE                    ((size_t) 1 << KM_BUDDY_MIN_ORDER)
#define KM_BUDDY_MAX_SIZE              ((size_t) 1 << (KM_BUDDY_MAX_ORDER - 1))

#define KM_BUDDY_CHUNK                 ((uint8_t) 1)
#define KM_BESTFIT_CHUNK               ((uint8_t) 0)



static DECLARE_ONADDED(km_on_region_added);
static DECLARE_ONREMOVED(km_on_region_removed);
static DECLARE_ONALLOCATE(km_on_allocate_region);
static DECLARE_ONFREE(km_on_free_region);

static DECLARE_ONALLOCATE(km_on_quantum_request);
static DECLARE_ONFREE(km_on_quantum_release);

static int km_segid;
static struct mem_nregion init_region;
static wtreeRoot_t km_pool;
static size_t km_sz;
static size_t km_free_sz;
static slistEntry_t  buddy_entries[KM_BUDDY_ORDER_COUNT];

struct km_alc_header {
	uint16_t    hsz;
	uint16_t    tsz;
};

static wt_adapter km_adapter = {
		.onadded = km_on_region_added,
		.onremoved = km_on_region_removed,
		.onallocate = km_on_allocate_region,
		.onfree = km_on_free_region
};


static void* km_reclaim_chunk_from_buddy(size_t sz);
static void* km_half_chunk(uint8_t* chunk, uint8_t buddy_idx);
static void km_split_chunk_rc(uint8_t*, uint8_t from_buddy_idx, uint8_t to_buddy_idx);

void init_km(int seg_id) {
	km_sz = km_free_sz = 0;
	km_segid = seg_id;

	uint8_t buddy_order;
	for(buddy_order = 0; buddy_order < KM_BUDDY_ORDER_COUNT; buddy_order++) {
		cdsl_slistEntryInit(&buddy_entries[buddy_order]);
	}

	wtree_rootInit(&km_pool, NULL, &km_adapter, 0);
	// allocate initial region for kernel allocator & map it into kernel memory management node
	if(tch_allocate_region(&init_region, KERNEL_DYNAMIC_SIZE, PERM_KERNEL_ALL | PERM_OTHER_RD) != tchOK) {
		KERNEL_PANIC("kernel memory allocator failure : No region available");
	}
	tch_map_region(current_mm, &init_region);
	wtreeNode_t* node = wtree_baseNodeInit(&km_pool, get_addr_from_page(init_region->poff), get_size_from_pcount(init_region->psize));
	wtree_addNode(&km_pool, node, FALSE, NULL);
}

void *km_alloc(uint32_t sz) {
	if(!sz) {
		return NULL;
	}
	if(sz < KM_MIN_SIZE) {
		sz = KM_MIN_SIZE;
	}
	sz += sizeof(struct km_alc_header);
	uint8_t* chunk;
	/***
	 *  allocate chunk
	 */
	if(sz > KM_BUDDY_MAX_SIZE) {
		chunk = wtree_reclaim_chunk(&km_pool, sz, TRUE);
	} else {
		chunk = km_reclaim_chunk_from_buddy(sz);
	}

	km_free_sz -= sz;
	*((uint32_t*) &chunk[sz - sizeof(uint32_t)]) = sz - sizeof(uint32_t);
	*((uint32_t*) chunk) = sz - sizeof(uint32_t);
	return (void*) &chunk[sizeof(uint32_t)];
}


tchStatus km_free(void* ptr) {
	if(!ptr) {
		return tchErrorParameter;
	}
	uint32_t* chunk = (uint32_t*) ptr;
	chunk--;
	uint32_t sz = chunk[0];
	if(sz != chunk[sz >> 2]) {
		return tchErrorHeapCorruption;
	}
	if(sz - sizeof(uint32_t) > KM_QUANTUM_MAX_SIZE) {
		if(!quantum_free_chunk(&km_quantum, chunk)) {
			return tchErrorHeapCorruption;
		}
	} else {
		wtreeNode_t* node = wtree_nodeInit(&km_pool, chunk, sz + sizeof(uint32_t), NULL);
		wtree_addNode(&km_pool, node, TRUE, NULL);
	}
	km_free_sz += (sz + sizeof(uint32_t));
	return tchOK;
}

void km_mstat(struct mstat_t* mstat) {
	mstat->total = km_sz;
	mstat->used = km_sz - km_free_sz;
	mstat->cached = 0;
}

static void* km_reclaim_chunk_from_buddy(size_t sz) {
	uint8_t cur_buddy, buddy_idx = KM_BUDDY_MIN_ORDER;
	void* chunk;

	while(sz < (1 << buddy_idx)) {
		buddy_idx++;
	}
	cur_buddy = buddy_idx;

	while(cdsl_slistIsEmpty(&buddy_entries[cur_buddy])) {
		cur_buddy++;
		/*
		if(cur_buddy == KM_BUDDY_MAX_ORDER) {
			// add new chunk from best fit
			chunk = wtree_reclaim_chunk(&km_pool, KM_BUDDY_MAX_SIZE, FALSE);
			// put chunk into max buddy
			while(cur_buddy != buddy_idx) {
				chunk = km_half_chunk(chunk, cur_buddy);
			}
			return chunk;
		}*/
	}
	chunk = cdsl_slistDequeue(&buddy_entries[buddy_idx]);
	return chunk;
}

static void* km_half_chunk(uint8_t* chunk, uint8_t buddy_idx) {
	uint32_t sz = (1 << buddy_idx);
	cdsl_slistNodeInit(chunk);
	cdsl_slistPutTail(&buddy_entries[buddy_idx - 1]);
	return chunk[sz  >> 1];
}


static void km_split_chunk_rc(uint8_t* chunk, uint8_t from_buddy_idx, uint8_t to_buddy_idx) {
}


static DECLARE_ONALLOCATE(km_on_quantum_request) {
	*rsz = total_sz;
	return km_alloc(total_sz);
}

static DECLARE_ONFREE(km_on_quantum_release) {
	return 0;
}

static DECLARE_ONADDED(km_on_region_added) {
	km_sz += node->size;
	km_free_sz += node->size;
}

static DECLARE_ONREMOVED(km_on_region_removed) {
	km_sz -= node->size;
	km_free_sz -= node->size;
}

static DECLARE_ONALLOCATE(km_on_allocate_region) {

}

static DECLARE_ONFREE(km_on_free_region) {

}


