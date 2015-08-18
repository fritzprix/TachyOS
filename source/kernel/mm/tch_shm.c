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

DECLARE_SYSCALL_1(shmem_alloc,size_t,void*);
DECLARE_SYSCALL_1(shmem_free,void*,tchStatus);
DECLARE_SYSCALL_0(shmem_avail,uint32_t);
DECLARE_SYSCALL_0(shmem_cleanup,tchStatus);


void tch_shm_init(int seg_id){
	if(seg_id < 0)
		KERNEL_PANIC("tch_shm.c","Segment given in initialization is not valid");

	shm_init_segid = seg_id;
	if(!(tch_segmentAllocRegion(seg_id,&shm_init_region,CONFIG_SHM_SIZE,PERM_OTHER_ALL | PERM_OWNER_ALL | PERM_KERNEL_ALL) > 0))
		KERNEL_PANIC("tch_shm.c","Can't allocate region for shmem");

	wt_heapNode_t* shm_node = (wt_heapNode_t*) kmalloc(sizeof(wt_heapNode_t));
	wt_initRoot(&shm_root);
	wt_initNode(shm_node,tch_getRegionBase(&shm_init_region), tch_getRegionSize(&shm_init_region));
	wt_addNode(&shm_root,shm_node);
}


DEFINE_SYSCALL_1(shmem_alloc,size_t,sz,void*){
	if(!sz)
		return NULL;
	struct kobj_header* chnk;
	size_t asz = sz + sizeof(struct kobj_header);
	if(!(wt_available(&shm_root) > asz)){
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
	cdsl_dlistPutHead(&tch_currentThread->kthread->mm.shm_list,&chnk->alc_ln);
	return &chnk[1];
}

DEFINE_SYSCALL_1(shmem_free,void*,ptr,tchStatus){
	if(!ptr)
		return tchErrorParameter;
	struct kobj_header* chnk = (struct kobj_header*) ptr;
	cdsl_dlistRemove(&chnk->alc_ln);
	if(wt_free(&shm_root,ptr) == WT_ERROR)
		tchk_schedTerminate(tch_currentThread,tchErrorHeapCorruption);
	return tchOK;
}

DEFINE_SYSCALL_0(shmem_avail,uint32_t){
	return wt_available(&shm_root);
}

DEFINE_SYSCALL_0(shmem_cleanup,tchStatus){
	struct kobj_entry* chnk;
	while((chnk = (struct kobj_entry*) cdsl_dlistDequeue(&current_mm->shm_list)) != NULL){
		chnk = (struct kobj_entry*) container_of(chnk,struct kobj_entry,alc_ln);
		if(wt_free(&shm_root,chnk) == WT_ERROR){
			// TODO :shmem corruption in thread termination
			KERNEL_PANIC("tch_shm.c","shmem is corrupted");
		}else{
			if(chnk->kobj.__destr_fn(&chnk->kobj) != tchOK){
				//TODO : deal with destructor error
				return tchErrorResource;
			}
		}
	}
	return tchOK;
}

void* tch_shmAlloc(size_t sz){
	if(tch_port_isISR())
		return __shmem_alloc(sz);
	else
		return (void*) __SYSCALL_1(shmem_alloc,sz);
}

tchStatus tch_shmFree(void* mchunk){
	if(tch_port_isISR())
		return __shmem_free(mchunk);
	else
		return __SYSCALL_1(shmem_free,mchunk);
}

uint32_t tch_shmAvali(){
	if(tch_port_isISR())
		return __shmem_avail();
	else
		return __SYSCALL_0(shmem_avail);
}

tchStatus tch_shmCleanUp(){
	if(tch_port_isISR())
		return tchErrorISR;
	else
		return __SYSCALL_0(shmem_cleanup);
}

