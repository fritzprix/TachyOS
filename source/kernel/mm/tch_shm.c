/*
 * tch_shm.c
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */


#include "tch_ktypes.h"
#include "tch_shm.h"
#include "tch_segment.h"
#include "tch_err.h"
#include "wtmalloc.h"

static struct mem_region 	shm_region;
static wt_heaproot_t		shm_cacheroot;

void tchk_shmInit(int seg_id){
	if(seg_id < 0)
		KERNEL_PANIC("tch_shm.c","Segment given in initialization is not valid");
	tch_segmentAllocRegion(seg_id,&shm_region,CONFIG_SHM_SIZE,PERM_OTHER_ALL | PERM_OWNER_ALL | PERM_KERNEL_ALL);

	wt_alloc_t* cache = (wt_alloc_t*) kmalloc(sizeof(wt_alloc_t));
	wtreeHeap_initCacheRoot(&shm_cacheroot);
	wtreeHeap_initCacheNode(cache,(uint8_t*)(shm_region.poff << CONFIG_PAGE_SHIFT), shm_region.psz << CONFIG_PAGE_SHIFT);
	wtreeHeap_addCache(&shm_cacheroot,cache);
}


void* tchk_shmalloc(size_t sz){
	if(!sz)
		return NULL;
	wtreeHeap_malloc()
}

void tchk_shmFree(void* mem){

}

uint32_t tchk_shmAvail(){

}

tchStatus tchk_shmCleanUp(tch_thread_kheader* owner){

}

void* tch_shmAlloc(size_t sz){

}

void tch_shmFree(void* mchunk){

}

uint32_t tch_shmAvali(){

}
