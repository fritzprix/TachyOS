/*
 * tch_mpool.c
 *
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *
 *  Created on: 2014. 7. 5.
 *      Author: innocentevil
 */



#include "tch_kernel.h"
#include "tch_lib.h"




typedef struct tch_mpool_header_t {
	void*                bend;
	void*                bfree;
	tch_mpoolDef_t*      bDef;
}tch_mpool_header_t;

static tch_mpool_id tch_mpool_create(const tch_mpoolDef_t* pool);
static void* tch_mpool_alloc(tch_mpool_id mpool);
static void* tch_mpool_calloc(tch_mpool_id mpool);
static tchStatus tch_mpool_free(tch_mpool_id mpool,void* block);

__attribute__((section(".data"))) static tch_mpool_ix MPoolStaticIntance = {
		tch_mpool_create,
		tch_mpool_alloc,
		tch_mpool_calloc,
		tch_mpool_free
};

const tch_mpool_ix* Mempool = &MPoolStaticIntance;


tch_mpool_id tch_mpool_create(const tch_mpoolDef_t* pool){
	tch_mpool_header_t* mp = (tch_mpool_header_t*) pool->pool - 1;
	tch_memset((uint8_t*)pool->pool,0,pool->align * pool->align);
	void* next = NULL;
	uint8_t* blk = (uint8_t*)pool->pool;
	uint8_t* end = blk + pool->align * pool->count;
	mp->bend = end;
	mp->bfree = blk;
	mp->bDef = (tch_mpoolDef_t*) pool;
	end = end - pool->align;
	while(1){
		next = blk + pool->align;
		if(next > end) break;
		*((void**)blk) = next;
		blk = (uint8_t*)next;
	}
	*((void**)blk) = 0;
	return mp;
}

void* tch_mpool_alloc(tch_mpool_id mpool){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	tch_port_kernel_lock();
	void** free = (void**)mp_header->bfree;
	if(free){
		mp_header->bfree = *free;
	}
	tch_port_kernel_unlock();
	return free;
}


void* tch_mpool_calloc(tch_mpool_id mpool){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	void* free = tch_mpool_alloc(mpool);
	if(free){
		tch_memset((uint8_t*)free,0,mp_header->bDef->align);
	}
	return free;
}

tchStatus tch_mpool_free(tch_mpool_id mpool,void* block){
	tch_mpool_header_t* mp_header = (tch_mpool_header_t*) mpool;
	if((block < mp_header->bDef->pool) || (block > mp_header->bend))
		return osErrorParameter;
	tch_port_kernel_lock();
	*((void**)block) = mp_header->bfree;
	mp_header->bfree = block;
	tch_port_kernel_unlock();
	return osOK;
}
