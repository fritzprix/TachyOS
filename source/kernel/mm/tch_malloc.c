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
		.available = tch_avail
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

size_t tch_avail(){
	return ((wt_cache_t*) current->cache)->size +
			((wt_heapRoot_t*) current->heap)->size;
}
