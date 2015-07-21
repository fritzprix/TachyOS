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

#define  MIN_CACHE_SIZE				(sizeof(struct mem_region) + sizeof(struct wt_heap_node))
typedef void (*obj_destr) (void* self);

struct obj_entry {
	cdsl_dlistNode_t		alc_ln;
};


static wt_heapRoot_t kernel_cache_root;
static wt_heapNode_t init_kernel_cache;
static struct mem_region init_region;

static int init_segid;


void tch_initKmalloc(int segid){

	init_segid = segid;
	tch_segmentAllocRegion(segid,&init_region,CONFIG_KERNEL_DYNAMICSIZE,PERM_KERNEL_ALL | PERM_OTHER_RD);
	tch_mapRegion(&init_mm,&init_region);

	wt_initRoot(&kernel_cache_root);
	wt_initNode(&init_kernel_cache,tch_getRegionBase(&init_region),tch_getRegionSize(&init_region));

	wt_addNode(&kernel_cache_root,&init_kernel_cache);

}

void* kmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct obj_entry* chunk = NULL;
	size_t asz = sz + sizeof(struct obj_entry);

	tch_port_atomic_begin();
	if(!(wt_available(&kernel_cache_root)  > (MIN_CACHE_SIZE + asz))){
		// try to allocate new memory region and add it to kernel heap
		struct mem_region *nregion = wt_malloc(&kernel_cache_root,sizeof(struct mem_region));
		wt_heapNode_t	*alloc = wt_malloc(&kernel_cache_root,sizeof(wt_heapNode_t));
		size_t rsz = tch_segmentGetFreeSize(init_segid);
		rsz = (rsz * PAGE_SIZE) > CONFIG_KERNEL_DYNAMICSIZE? (rsz * PAGE_SIZE) : CONFIG_KERNEL_DYNAMICSIZE;
		if(rsz < sz){
			tch_port_atomic_end();
			return NULL;		// not able to satisfies memory request
								// otherwise, allocate new region and add it to kernel heap
		}
		tch_segmentAllocRegion(init_segid,nregion,rsz,PERM_KERNEL_ALL | PERM_OTHER_RD);
		wt_initNode(&init_kernel_cache,tch_getRegionBase(nregion),tch_getRegionSize(nregion));
		wt_addNode(&kernel_cache_root,&init_kernel_cache);
	}
	tch_port_atomic_end();

	tch_port_atomic_begin();
	chunk = wt_malloc(&kernel_cache_root,sz + sizeof(struct obj_entry));
	tch_port_atomic_end();

	if(!chunk){
		return NULL;
	}
	cdsl_dlistPutHead(&tch_currentThread->kthread->t_mm->alc_list,&chunk->alc_ln);			// add alloc list
	return (void*) ((size_t) chunk + sizeof(struct obj_entry));
}

void kfree(void* p){
	if(!p)
		return;
	struct obj_entry* obj_entry = (struct obj_entry*) ((size_t) p - sizeof(struct obj_entry));
	tch_port_atomic_begin();
	wt_free(&kernel_cache_root,obj_entry);
	tch_port_atomic_end();

	cdsl_dlistRemove(&obj_entry->alc_ln);
}

