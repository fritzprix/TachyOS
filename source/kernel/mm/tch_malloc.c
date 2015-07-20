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
	tch_mtxId mtx = tch_currentThread->t_kthread->t_mm->mtx;
	if(Mtx->lock(mtx,tchWaitForever) != tchOK)
		return NULL;
	result = wt_malloc(tch_currentThread->t_cache,sz);
	Mtx->unlock(mtx);
	return result;
}

void tch_free(void* ptr){
	if(!ptr)
		return;
	tch_mtxId mtx = tch_currentThread->t_kthread->t_mm->mtx;
	if(Mtx->lock(mtx,tchWaitForever) != tchOK)
		return;
	wt_free(tch_currentThread->t_cache,ptr);
	Mtx->unlock(mtx);
}

size_t tch_avail(){
	size_t res_sz = 0;
	tch_mtxId mtx = tch_currentThread->t_kthread->t_mm->mtx;
	if(Mtx->lock(mtx,tchWaitForever) != tchOK)
		return 0;
	res_sz = wt_available(tch_currentThread->t_cache);
	Mtx->unlock(mtx);
	return res_sz;
}
