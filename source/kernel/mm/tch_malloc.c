/*
 * tch_malloc.c
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */




#include "tch_kernel.h"
#include "tch_malloc.h"

const tch_mem_ix UserHeapInterface = {
		.alloc = tch_malloc,
		.free = tch_free,
		.mstat = tch_mstat
};

const tch_mem_ix* uMem = &UserHeapInterface;

void* tch_malloc(size_t sz){
	void* result;
	if(!sz)
		return NULL;
	result = wt_cacheMalloc(current->cache, sz);
	if (!result) {
		tch_mtxId mtx = current->mtx;
		if (Mtx->lock(mtx, tchWaitForever) != tchOK)
			return NULL;
		result = wt_malloc(current->heap, sz);
		Mtx->unlock(mtx);
	}
	return result;
}

void tch_free(void* ptr){
	if(!ptr)
		return;
	int result;
	if(WT_OK == (result = wt_cacheFree(current->cache,ptr)))
		return;
	if(result == WT_ERROR)
		goto ERR_HEAP_FREE;
	tch_mtxId mtx = current->mtx;
	if(Mtx->lock(mtx,tchWaitForever) != tchOK)
		return;
	result = wt_free(current->heap,ptr);
	Mtx->unlock(mtx);
	if(result == WT_ERROR)
		goto ERR_HEAP_FREE;
	return;

ERR_HEAP_FREE :
	tch_kernel_raise_error(current,tchErrorHeapCorruption,"heap corrupted");
}


void tch_mstat(mstat* statp){
	if(!statp)
		return;

	wt_heapRoot_t* uheap = (wt_heapRoot_t*) current->heap;

	statp->total = uheap->size;
	statp->used = uheap->size - uheap->free_sz;
	statp->cached = ((wt_cache_t* ) current->cache)->size;
}
