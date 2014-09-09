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

#include <stdlib.h>

#include "tch_kernel.h"


#define TCH_MPOOL_CLASS_KEY             ((uint16_t) 0x2D05)
#define tch_mpoolValidate(mpcb)         ((tch_mpool_cb*) mpcb)->bstate = ((uint32_t) mpcb & 0xFFFF) ^ TCH_MPOOL_CLASS_KEY
#define tch_mpoolInvalidate(mpcb)       ((tch_mpool_cb*) mpcb)->bstate &= ~(0xFFFF)
#define tch_mpoolIsValid(mpcb)          (((tch_mpool_cb*) mpcb)->bstate & 0xFFFF) == (((uint32_t) mpcb & 0xFFFF) ^ TCH_MPOOL_CLASS_KEY)


typedef struct tch_mpool_cb_t {
	uint32_t             bstate;
	void*                bend;
	void*                bfree;
	uint32_t             bcount;
	uint32_t             balign;
	void*                bpool;
}tch_mpool_cb;

static tch_mpoolId tch_mpool_create(size_t sz,uint32_t plen);
static void* tch_mpool_alloc(tch_mpoolId mpool);
static void* tch_mpool_calloc(tch_mpoolId mpool);
static tchStatus tch_mpool_free(tch_mpoolId mpool,void* block);
static tchStatus tch_mpool_destroy(tch_mpoolId mpool);


__attribute__((section(".data"))) static tch_mpool_ix MPoolStaticIntance = {
		tch_mpool_create,
		tch_mpool_alloc,
		tch_mpool_calloc,
		tch_mpool_free,
		tch_mpool_destroy
};

const tch_mpool_ix* Mempool = &MPoolStaticIntance;

tch_mpoolId tch_mpool_create(size_t sz,uint32_t plen){
	tch_mpool_cb* mpcb = (tch_mpool_cb*) Mem->alloc(sizeof(tch_mpool_cb) + sz * plen);
	mpcb->bpool = (tch_mpool_cb*) mpcb + 1;
	mpcb->bcount = plen;
	mpcb->balign = sz;
	memset(mpcb->bpool,0,sz * plen);
	void* next = NULL;
	uint8_t* blk = (uint8_t*) mpcb->bpool;
	uint8_t* end = blk + sz * plen;
	mpcb->bend = end;
	mpcb->bfree = blk;
	end = end - mpcb->balign;
	while(1){
		next = blk + mpcb->balign;
		if(next > (void*)end) break;
		*((void**) blk) = next;
		blk = (uint8_t*) next;
	}
	*((void**) blk) = 0;
	tch_mpoolValidate(mpcb);
	return (tch_mpoolId) mpcb;
}


void* tch_mpool_alloc(tch_mpoolId mpool){
	if(!tch_mpoolIsValid(mpool))
		return NULL;
	tch_mpool_cb* mpcb = (tch_mpool_cb*) mpool;
	if(!mpcb->bfree)
		return NULL;
	tch_port_kernel_lock();
	void** free = (void**)mpcb->bfree;
	if(free){
		mpcb->bfree = *free;
	}
	tch_port_kernel_unlock();
	return free;
}


void* tch_mpool_calloc(tch_mpoolId mpool){
	tch_mpool_cb* mpcb = (tch_mpool_cb*) mpool;
	void* free = tch_mpool_alloc(mpool);
	if(free){
		memset(free,0,mpcb->balign);
	}
	return free;
}

tchStatus tch_mpool_free(tch_mpoolId mpool,void* block){
	tch_mpool_cb* mpcb = (tch_mpool_cb*) mpool;
	if((block < mpcb->bpool) || (block > mpcb->bend))
		return osErrorParameter;
	tch_port_kernel_lock();
	*((void**)block) = mpcb->bfree;
	mpcb->bfree = block;
	tch_port_kernel_unlock();
	return osOK;
}


static tchStatus tch_mpool_destroy(tch_mpoolId mpool){
	if(!tch_mpoolIsValid(mpool)){
		return osErrorParameter;
	}
	tch_mpoolInvalidate(mpool);
	tch_mpool_cb* mcb = (tch_mpool_cb*) mpool;
	Mem->free(mcb);
	return osOK;
}
