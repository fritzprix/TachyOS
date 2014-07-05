/*
 * tch_mpool.c
 *
 *  Created on: 2014. 7. 5.
 *      Author: innocentevil
 */



#include "kernel/tch_kernel.h"



typedef struct tch_mpool_header_t {
	uint32_t             blastIdx;
	uint32_t             ballocCnt;
	tch_mpoolDef_t*      bDef;
}tch_mpool_header_t;

static tch_mpool_id tch_mpool_create(const tch_mpoolDef_t* pool);
static void* tch_mpool_alloc(tch_mpool_id mpool);
static void* tch_mpool_calloc(tch_mpool_id mpool);
static osStatus tch_mpool_free(tch_mpool_id mpool,void* block);

__attribute__((section(".data"))) static tch_mpool_ix MPoolStaticIntance = {
		tch_mpool_create,
		tch_mpool_alloc,
		tch_mpool_calloc,
		tch_mpool_free
};

const tch_mpool_ix* Mempool = &MPoolStaticIntance;


tch_mpool_id tch_mpool_create(const tch_mpoolDef_t* pool){
	tch_memset(pool->pool,pool->align * pool->align,0);
	tch_mpool_header_t* mpheader = (tch_mpool_header_t*) pool->pool - 1;
	mpheader->bDef = pool;
	mpheader->ballocCnt = 0;
	mpheader->blastIdx = 0;
	return (tch_mpool_id) mpheader;
}

void* tch_mpool_alloc(tch_mpool_id mpool){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	if(mp_header->ballocCnt >= mp_header->bDef->count)
		return NULL;
	tch_port_kernel_lock();
	mp_header->ballocCnt++;
	uint8_t* bp = mp_header->bDef->pool + (mp_header->bDef->align * mp_header->blastIdx++);
	if(mp_header->blastIdx == mp_header->bDef->count){
		mp_header->blastIdx = 0;
	}
	tch_port_kernel_unlock();
	return bp;
}

void* tch_mpool_calloc(tch_mpool_id mpool){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	if(mp_header->ballocCnt >= mp_header->bDef->count)
		return NULL;
	tch_port_kernel_lock();
	mp_header->ballocCnt++;
	uint8_t* bp = mp_header->bDef->pool + (mp_header->bDef->align * mp_header->blastIdx++);
	if(mp_header->blastIdx == mp_header->bDef->count){
		mp_header->blastIdx = 0;
	}
	tch_port_kernel_unlock();
	tch_memset(bp,mp_header->bDef->align,0);
	return bp;
}

osStatus tch_mpool_free(tch_mpool_id mpool,void* block){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	if(mp_header->ballocCnt <= 0)
		return osErrorValue;
	if((mp_header->bDef->pool <= block) && ((mp_header->bDef->pool + mp_header->bDef->align * mp_header->bDef->count) > mpool)){
		mp_header->ballocCnt--;
		return osOK;
	}
	return osErrorParameter;
}
