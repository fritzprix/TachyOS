/*
 * tch_kmalloc.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: Jul 7, 2015
 *      Author: innocentevil
 */

#include "tch_segment.h"
#include "tch_kernel.h"
#include "tch_kmalloc.h"

#include "../../../include/kernel/mm/owtmalloc.h"
#include "wtree.h"
#include "cdsl_dlist.h"
#include "tch_err.h"
#include "tch_mm.h"

#define MIN_CACHE_SIZE				(sizeof(struct mem_region) + sizeof(wtreeNode_t))
#define available(heap) 			(((wt_heapRoot_t*) heap)->free_sz)
#define KM_PERMISSION               (PERM_KERNEL_ALL | PERM_OTHER_RD)

static DECLARE_ONALLOCATE(km_onallocate);
static DECLARE_ONFREE(km_onfree);
static DECLARE_ONREMOVED(km_onremoved);
static DECLARE_ONADDED(km_onadded);

struct alloc_header {
	dlistNode_t      alc_ln;
	size_t           size;
};


struct map_header {
	wtreeNode_t         wtnode;
	struct mem_region   mreg;
};


static wt_adapter kmalloc_adapter = {
		.onallocate = km_onallocate,
		.onfree = km_onfree,
		.onremoved = km_onremoved,
		.onadded = km_onadded
};



static wtreeRoot_t kernel_heap_root_new;
static struct mem_region init_region;

static int init_segid;


void tch_kmalloc_init(int segid){

	init_segid = segid;
	tch_segmentAllocRegion(segid, &init_region,KERNEL_DYNAMIC_SIZE, KM_PERMISSION);

	wtree_rootInit(&kernel_heap_root_new, NULL, &kmalloc_adapter, sizeof(struct mem_region));
	wtreeNode_t* init_node = wtree_baseNodeInit(&kernel_heap_root_new, (uaddr_t) tch_getRegionBase(&init_region), tch_getRegionSize(&init_region));
	wtree_addNode(&kernel_heap_root_new, init_node, FALSE, NULL);
}

void* kmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct alloc_header* chunk = NULL;
	size_t asz = sz + sizeof(struct alloc_header);

	tch_port_atomicBegin();
	chunk = wtree_reclaim_chunk(&kernel_heap_root_new, asz, TRUE);
	if(!chunk){
		tch_port_atomicEnd();
		return NULL;
	}
	chunk->size = asz;
	cdsl_dlistNodeInit(&chunk->alc_ln);
	cdsl_dlistPutHead((dlistEntry_t*) &current_mm->alc_list,&chunk->alc_ln);			// add alloc list
	tch_port_atomicEnd();

	return (void*) ((size_t) chunk + sizeof(struct alloc_header));
}

void kfree(void* p){
	if(!p)
		return;
	int result;
	struct alloc_header* obj_entry = (struct alloc_header*) p;
	obj_entry--;
	tch_port_atomicBegin();
	cdsl_dlistRemove(&obj_entry->alc_ln);
	wtreeNode_t* node = wtree_nodeInit(&kernel_heap_root_new, obj_entry, obj_entry->size, NULL);
	wtree_addNode(&kernel_heap_root_new, node, TRUE, NULL);
	tch_port_atomicEnd();
}

void kmstat(mstat* sp){
	if(!sp)
		return;
	sp->cached = 0;
	sp->total = wtree_totalSize(&kernel_heap_root_new);
	sp->used = wtree_freeSize(&kernel_heap_root_new);
}


static DECLARE_ONALLOCATE(km_onallocate) {
	struct mem_region mreg;
	int pcount = tch_segmentAllocRegion(init_segid, &mreg, total_sz, KM_PERMISSION);
	if(pcount <= 0) {
		KERNEL_PANIC("Out-Of-Memory\n");
	}
	return (void*) (mreg.poff << PAGE_OFFSET);
}

static DECLARE_ONFREE(km_onfree) {
	struct mem_segment* pseg = tch_segmentLookup(init_segid);
	if(wtnode->base_size) {
		struct mem_region* free_reg = (struct mem_region*) &wtnode[1];
		tch_segmentFreeRegion(free_reg, TRUE);
	}
	return 0;
}

static DECLARE_ONREMOVED(km_onremoved) {
	if(node->base_size) {
		struct map_header* hdr = container_of(node, struct map_header, wtnode);
		tch_unmapRegion(&init_mm, &hdr->mreg);
		if(merged) {
			tch_segmentFreeRegion(&hdr->mreg, FALSE);
		}
	}
}

static DECLARE_ONADDED(km_onadded) {
	if(node->base_size) {
		struct map_header* hdr = container_of(node, struct map_header, wtnode);
		struct mem_segment* seg = tch_segmentLookup(init_segid);
		tch_initRegion(&hdr->mreg, seg, (size_t) (node->top - node->base_size) >> PAGE_OFFSET, node->base_size >> PAGE_OFFSET, KM_PERMISSION);
		tch_mapRegion(&init_mm, &hdr->mreg);
	}
}
