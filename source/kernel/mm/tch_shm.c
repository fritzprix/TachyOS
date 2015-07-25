/*
 * tch_shm.c
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


void tch_shmInit(int seg_id){
	if(seg_id < 0)
		KERNEL_PANIC("tch_shm.c","Segment given in initialization is not valid");

	shm_init_segid = seg_id;
	tch_segmentAllocRegion(seg_id,&shm_init_region,CONFIG_SHM_SIZE,PERM_OTHER_ALL | PERM_OWNER_ALL | PERM_KERNEL_ALL);

	wt_heapNode_t* cache = (wt_heapNode_t*) kmalloc(sizeof(wt_heapNode_t));

	wt_initRoot(&shm_root);
	wt_initNode(cache,tch_getRegionBase(&shm_init_region), tch_getRegionSize(&shm_init_region));
	wt_addNode(&shm_root,cache);
}

void* tchk_shmalloc(size_t sz){
	if(!sz)
		return NULL;
	struct kobj_header* chnk;
	size_t asz = sz + sizeof(struct kobj_header);
	if(!(wt_available(&shm_root) > asz)){
		struct mem_region* nregion = (struct mem_region*) kmalloc(sizeof(struct mem_region));
		wt_heapNode_t* shm_node = (wt_heapNode_t*) kmalloc(sizeof(wt_heapNode_t));
		if(!nregion || !shm_node)
			goto RETURN_FAIL;
		if(!(tch_segmentAllocRegion(shm_init_segid,nregion,asz,PERM_KERNEL_ALL | PERM_OWNER_ALL | PERM_OTHER_ALL) > 0))
			goto RETURN_FAIL;

		wt_initNode(shm_node,tch_getRegionBase(nregion),tch_getRegionSize(nregion));
		wt_addNode(&shm_root,shm_node);

	RETURN_FAIL:
		kfree(nregion);
		kfree(shm_node);
		return NULL;
	}

	chnk = wt_malloc(&shm_root,asz);
	cdsl_dlistPutHead(&tch_currentThread->kthread->t_mm->shm_list,&chnk->alc_ln);
	return &chnk[1];
}

void tchk_shmFree(void* mem){
	if(!mem)
		return;
	struct kobj_header* chnk = (struct kobj_header*) mem;
	cdsl_dlistRemove(&chnk->alc_ln);
	if(wt_free(&shm_root,mem) == WT_ERROR)
		tchk_schedThreadTerminate(tch_currentThread,tchErrorHeapCorruption);
}

uint32_t tchk_shmAvail(){
	return wt_available(&shm_root);
}

tchStatus tchk_shmCleanUp(struct tch_mm* owner){
	if(!owner)
		return tchErrorParameter;
	struct kobj_header* chnk;
	while((chnk = (struct kobj_header*) cdsl_dlistDequeue(&owner->shm_list)) != NULL){
		if(wt_free(&shm_root,chnk) == WT_ERROR){
			// TODO : handle shmem corruption in thread termination (this function intended to be called from thread termination)

		}
	}
}

void* tch_shmAlloc(size_t sz){

}

void tch_shmFree(void* mchunk){

}

uint32_t tch_shmAvali(){

}
