/*
 * tch_shm.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_ktypes.h"
#include "tch_shm.h"
#include "tch_segment.h"
#include "tch_err.h"
#include "tch_mm.h"
#include "wtmalloc.h"


#define  MIN_CACHE_SIZE				(sizeof(struct mem_region) + sizeof(struct wt_heap_node))


static struct mem_region 	shm_init_region;
static wt_heapRoot_t		shm_root;
static int 					shm_init_segid;

DECLARE_SYSCALL_1(shmem_alloc,size_t,void*);
DECLARE_SYSCALL_1(shmem_free,void*,tchStatus);
DECLARE_SYSCALL_1(shmem_cleanup,tch_threadId,tchStatus);
DECLARE_SYSCALL_1(shmem_statm,mstat*,tchStatus);

struct shmalloc_header {
	cdsl_dlistNode_t	alc_ln;
};

void tch_shmInit(int seg_id){
	if(seg_id < 0)
		KERNEL_PANIC("tch_shm.c","Segment given in initialization is not valid");

	shm_init_segid = seg_id;
	if(!(tch_segmentAllocRegion(seg_id,&shm_init_region,SHM_SIZE,PERM_OTHER_ALL | PERM_OWNER_ALL | PERM_KERNEL_ALL) > 0))
		KERNEL_PANIC("tch_shm.c","Can't allocate region for shmem");

	wt_heapNode_t* shm_node = (wt_heapNode_t*) kmalloc(sizeof(wt_heapNode_t));
	wt_initRoot(&shm_root);
	wt_initNode(shm_node,tch_getRegionBase(&shm_init_region), tch_getRegionSize(&shm_init_region));
	wt_addNode(&shm_root,shm_node);
}


DEFINE_SYSCALL_1(shmem_alloc,size_t,sz,void*){
	if(!sz)
		return NULL;
	struct shmalloc_header* chnk;
	size_t asz = sz + sizeof(struct shmalloc_header);
	if(!(shm_root.free_sz > asz)){
		struct mem_region* nregion = (struct mem_region*) kmalloc(sizeof(struct mem_region));
		wt_heapNode_t* shm_node = (wt_heapNode_t*) kmalloc(sizeof(wt_heapNode_t));

		if(!nregion || !shm_node)
			goto RETURN_FAIL;
		if(!(tch_segmentAllocRegion(shm_init_segid,nregion,asz,PERM_KERNEL_ALL | PERM_OWNER_ALL | PERM_OTHER_ALL) > 0)){
			//TODO : deal with shmem depletion
			goto RETURN_FAIL;
		}

		wt_initNode(shm_node,tch_getRegionBase(nregion),tch_getRegionSize(nregion));
		wt_addNode(&shm_root,shm_node);

	RETURN_FAIL:
		kfree(nregion);
		kfree(shm_node);
		return NULL;
	}

	chnk = wt_malloc(&shm_root,asz);
	cdsl_dlistInit(&chnk->alc_ln);
	cdsl_dlistPutHead(&current->kthread->mm.shm_list,&chnk->alc_ln);
	return &chnk[1];
}

DEFINE_SYSCALL_1(shmem_free,void*,ptr,tchStatus){
	if(!ptr)
			return tchErrorParameter;
	struct shmalloc_header* chnk = (struct shmalloc_header*) ptr;
	chnk--;
	cdsl_dlistRemove(&chnk->alc_ln);
	if (wt_free(&shm_root, chnk) == WT_ERROR)
		tch_schedTerminate(current, tchErrorHeapCorruption);
	return tchOK;
}

DEFINE_SYSCALL_1(shmem_cleanup,tch_threadId,tid,tchStatus){
	tch_thread_kheader* kth = getThreadKHeader(tid);
	cdsl_dlistNode_t* shm_alc = &kth->mm.shm_list;
	struct shmalloc_header* chnk = NULL;
	while(!cdsl_dlistIsEmpty(shm_alc)){
		 chnk = (struct shmalloc_header*) cdsl_dlistDequeue(shm_alc);
		 if(!chnk)
			 return tchErrorHeapCorruption;
		 chnk = container_of(chnk,struct shmalloc_header,alc_ln);
		 if(wt_free(&shm_root,chnk) == WT_ERROR)
			 return tchErrorHeapCorruption;
	}
	return tchOK;
}


DEFINE_SYSCALL_1(shmem_statm,mstat*,statp,tchStatus){
	if(!statp)
		return tchErrorParameter;
	statp->total = shm_root.size;
	statp->used = shm_root.size - shm_root.free_sz;
	statp->cached = 0;
	return tchOK;
}


void* tch_shmAlloc(size_t sz){
	if(tch_port_isISR())
		return __shmem_alloc(sz);
	return (void*) __SYSCALL_1(shmem_alloc,sz);
}


tchStatus tch_shmFree(void* mchunk){
	if(tch_port_isISR())
		return __shmem_free(mchunk);
	return __SYSCALL_1(shmem_free,mchunk);
}


void tch_shmStat(mstat* stat) {
	if(tch_port_isISR())
		__shmem_statm(stat);
	__SYSCALL_1(shmem_statm,stat);
}


tchStatus tch_shmCleanup(tch_threadId tid) {
	if(tch_port_isISR())
		return __shmem_cleanup(tid);
	return __SYSCALL_1(shmem_cleanup,tid);
}

