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
#include "cdsl_dlist.h"
#include "tch_err.h"
#include "tch_mm.h"

#define MIN_CACHE_SIZE				(sizeof(struct mem_region) + sizeof(struct wt_heap_node))
#define available(heap) 			(((wt_heapRoot_t*) heap)->free_sz)

static wt_heapRoot_t kernel_heap_root;
static wt_heapNode_t init_kernel_cache;
static struct mem_region init_region;

static int init_segid;

struct alloc_header {
	dlistNode_t      alc_ln;
};


void tch_kmalloc_init(int segid){

	init_segid = segid;
	tch_segmentAllocRegion(segid, &init_region, KERNEL_DYNAMIC_SIZE, PERM_KERNEL_ALL | PERM_OTHER_RD);
	tch_mapRegion(&init_mm,&init_region);

	wt_initRoot(&kernel_heap_root);
	wt_initNode(&init_kernel_cache,tch_getRegionBase(&init_region),tch_getRegionSize(&init_region));
	wt_addNode(&kernel_heap_root,&init_kernel_cache);
}

void* kmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct alloc_header* chunk = NULL;
	size_t asz = sz + sizeof(struct alloc_header);

	tch_port_atomicBegin();
	if(!(available(&kernel_heap_root)  > (MIN_CACHE_SIZE + asz))){
		// try to allocate new memory region and add it to kernel heap
		struct mem_region *nregion = wt_malloc(&kernel_heap_root,sizeof(struct mem_region));
		wt_heapNode_t	*alloc = wt_malloc(&kernel_heap_root,sizeof(wt_heapNode_t));
		size_t rsz = tch_segmentGetFreeSize(init_segid);
		if((rsz * PAGE_SIZE) < sz){
			tch_port_atomicEnd();
			return NULL;		// not able to satisfies memory request
								// allocate new region and add it to kernel heap
		}

		rsz = ((rsz * PAGE_SIZE) > KERNEL_DYNAMIC_SIZE) ? (rsz * PAGE_SIZE) : KERNEL_DYNAMIC_SIZE;
		tch_segmentAllocRegion(init_segid,nregion,rsz,PERM_KERNEL_ALL | PERM_OTHER_RD);
		wt_initNode(&init_kernel_cache,tch_getRegionBase(nregion),tch_getRegionSize(nregion));
		wt_addNode(&kernel_heap_root,&init_kernel_cache);
	}
	chunk = wt_malloc(&kernel_heap_root,sz + sizeof(struct alloc_header));
	tch_port_atomicEnd();

	if(!chunk){
		return NULL;
	}
	cdsl_dlistNodeInit(&chunk->alc_ln);
	cdsl_dlistPutHead((dlistEntry_t*) &current_mm->alc_list,&chunk->alc_ln);			// add alloc list
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
	result = wt_free(&kernel_heap_root,obj_entry);
	tch_port_atomicEnd();
	if(result == WT_ERROR)
		KERNEL_PANIC("kernel heap corrupted");
}

void kmstat(mstat* sp){
	if(!sp)
		return;
	sp->cached = 0;
	sp->total = kernel_heap_root.size;
	sp->used = kernel_heap_root.size - kernel_heap_root.free_sz;
}

