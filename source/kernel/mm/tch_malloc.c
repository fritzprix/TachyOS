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
	result = wt_cacheMalloc(tch_currentThread->t_cache, sz);
	if (!result) {
		tch_mtxId mtx = tch_currentThread->kthread->t_mm->dynamic->mtx;
		if (Mtx->lock(mtx, tchWaitForever) != tchOK)
			return NULL;
		result = wt_malloc(tch_currentThread->kthread->t_mm->dynamic->heap, sz);
		Mtx->unlock(mtx);
	}
	return result;
}

void tch_free(void* ptr){
	if(!ptr)
		return;
	int result;
	if(WT_OK == (result = wt_cacheFree(tch_currentThread->t_cache,ptr)))
		return;
	if(result == WT_ERROR)
		goto ERR_HEAP_FREE;
	tch_mtxId mtx = tch_currentThread->kthread->t_mm->dynamic->mtx;
	if(Mtx->lock(mtx,tchWaitForever) != tchOK)
		return;
	result = wt_free(tch_currentThread->kthread->t_mm->dynamic->heap,ptr);
	Mtx->unlock(mtx);
	if(result == WT_ERROR)
		goto ERR_HEAP_FREE;
	return;

ERR_HEAP_FREE :
	tch_kernel_raise_error(tch_currentThread,tchErrorHeapCorruption,"heap corrupted");

}

size_t tch_avail(){
	return ((wt_cache_t*) tch_currentThread->t_cache)->size +
			((wt_heapRoot_t*) tch_currentThread->kthread->t_mm->dynamic->heap)->size;
}
