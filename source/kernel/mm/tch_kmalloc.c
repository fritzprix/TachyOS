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
#include "cdsl_slist.h"


typedef void (*obj_destr) (void* self);

struct mem_cache {
	cdsl_slistNode_t	ln;
	wt_alloc_t			alloc;
};

struct obj_wrapper {
	cdsl_slistNode_t	aln;
};

struct obj_entry {
	struct obj_wrapper		wrap;
	obj_destr				dtr;
};


static struct mem_cache	init_cache;
static struct mem_region init_region;


void tch_initKmalloc(int init_segid){
	tch_allocRegion(init_segid,&init_region,CONFIG_KERNEL_DYNAMICSIZE,PERM_KERNEL_ALL | PERM_OTHER_RD);
	tch_mapRegion(&init_mm,&init_region);

	cdsl_slistInit(&init_cache.ln);
	wtreeHeap_init(&init_cache.alloc,tch_getRegionBase(&init_region),tch_getRegionSize(&init_region));
}

void* kmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct mem_cache* cache = &init_cache;
	struct obj_wrapper* chunk = NULL;
	while((chunk = wtreeHeap_malloc(&cache->alloc,sz + sizeof(struct obj_wrapper))) == NULL){
		cache = cache->ln.next;
		if(!cache){

			return NULL;
		}
		cache = container_of(cache,struct mem_cache,ln);
	}
	cdsl_slistPutHead(&init_mm.alc_list,&chunk->aln);		// add alloc list
	return (void*) ((size_t) chunk + sizeof(struct obj_wrapper));
}

void kfree(void* p){

}
