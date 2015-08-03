/*
 * tch_kmalloc.c
 *
 *  Created on: Jul 7, 2015
 *      Author: innocentevil
 */

#include "tch_segment.h"
#include "tch_kernel.h"
#include "tch_kmalloc.h"
#include "wtmalloc.h"
#include "cdsl_dlist.h"
#include "tch_err.h"
#include "tch_mm.h"

#define  MIN_CACHE_SIZE				(sizeof(struct mem_region) + sizeof(struct wt_heap_node))


static wt_heapRoot_t kernel_heap_root;
static wt_heapNode_t init_kernel_cache;
static struct mem_region init_region;

static int init_segid;


void tch_kmalloc_init(int segid){



	init_segid = segid;
	tch_segmentAllocRegion(segid,&init_region,CONFIG_KERNEL_DYNAMICSIZE,PERM_KERNEL_ALL | PERM_OTHER_RD);
	tch_mapRegion(&init_mm,&init_region);

	wt_initRoot(&kernel_heap_root);
	wt_initNode(&init_kernel_cache,tch_getRegionBase(&init_region),tch_getRegionSize(&init_region));

	wt_addNode(&kernel_heap_root,&init_kernel_cache);

}

void* kmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct kobj_header* chunk = NULL;
	size_t asz = sz + sizeof(struct kobj_header);

	tch_port_atomicBegin();
	if(!(wt_available(&kernel_heap_root)  > (MIN_CACHE_SIZE + asz))){
		// try to allocate new memory region and add it to kernel heap
		struct mem_region *nregion = wt_malloc(&kernel_heap_root,sizeof(struct mem_region));
		wt_heapNode_t	*alloc = wt_malloc(&kernel_heap_root,sizeof(wt_heapNode_t));
		size_t rsz = tch_segmentGetFreeSize(init_segid);
		if(rsz < sz){
			tch_port_atomicEnd();
			return NULL;		// not able to satisfies memory request
								// otherwise, allocate new region and add it to kernel heap
		}

		rsz = (rsz * PAGE_SIZE) > CONFIG_KERNEL_DYNAMICSIZE? (rsz * PAGE_SIZE) : CONFIG_KERNEL_DYNAMICSIZE;
		tch_segmentAllocRegion(init_segid,nregion,rsz,PERM_KERNEL_ALL | PERM_OTHER_RD);
		wt_initNode(&init_kernel_cache,tch_getRegionBase(nregion),tch_getRegionSize(nregion));
		wt_addNode(&kernel_heap_root,&init_kernel_cache);
	}
	tch_port_atomicEnd();

	tch_port_atomicBegin();
	chunk = wt_malloc(&kernel_heap_root,sz + sizeof(struct kobj_header));
	tch_port_atomicEnd();

	if(!chunk){
		return NULL;
	}
	cdsl_dlistPutHead(&current_mm->alc_list,&chunk->alc_ln);			// add alloc list
	return (void*) ((size_t) chunk + sizeof(struct kobj_header));
}

void kfree(void* p){
	if(!p)
		return;
	int result;
	struct kobj_entry* obj_entry = (struct kobj_entry*) container_of(p,struct kobj_entry,kobj);
	tch_port_atomicBegin();
	result = wt_free(&kernel_heap_root,obj_entry);
	tch_port_atomicEnd();
	if(result == WT_ERROR)
		KERNEL_PANIC("tch_kmalloc.c","kernel heap corrupted");
	cdsl_dlistRemove(&obj_entry->alc_ln);
}

