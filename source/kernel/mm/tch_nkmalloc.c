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

#ifndef KM_MIN_SIZE
#define KM_MIN_SIZE                    ((size_t) 2)
#endif

#ifndef KM_QUANTUM_MAX_SIZE
#define KM_QUANTUM_MAX_SIZE             ((size_t) 32)
#endif

static DECLARE_ONADDED(km_on_region_added);
static DECLARE_ONREMOVED(km_on_region_removed);
static DECLARE_ONALLOCATE(km_on_allocate_region);
static DECLARE_ONFREE(km_on_free_region);

static DECLARE_ONALLOCATE(km_on_quantum_request);
static DECLARE_ONFREE(km_on_quantum_release);

static int km_segid;
static struct mem_nregion init_region;
static wtreeRoot_t km_pool;
static quantumRoot_t km_quantum;
static size_t km_sz;
static size_t km_free_sz;

struct km_heap_header {
	uint32_t         psz;
	uint32_t         nsz;
};

static wt_adapter km_adapter = {
		.onadded = km_on_region_added,
		.onremoved = km_on_region_removed,
		.onallocate = km_on_allocate_region,
		.onfree = km_on_free_region
};

void init_km(int seg_id) {
	km_sz = km_free_sz = 0;
	km_segid = seg_id;
	wtree_rootInit(&km_pool, NULL, &km_adapter, 0);

	// initialize quantum allocator
	quantum_root_init(&km_quantum,km_on_quantum_request, km_on_quantum_release);

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
	uint8_t* chunk;
	if(sz > KM_QUANTUM_MAX_SIZE) {
		sz = sz + sizeof(struct km_heap_header);
		chunk = wtree_reclaim_chunk(&km_pool, sz, TRUE);
	} else {
		sz = sz + sizeof(struct km_heap_header);
		chunk = quantum_reclaim_chunk(km_quantum, sz);
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


