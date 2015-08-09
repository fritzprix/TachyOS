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


typedef struct tch_mpoolCb {
	tch_kobj             __obj;
	uint32_t             bstate;
	void*                bend;
	void*                bfree;
	uint32_t             balign;
	void*                bpool;
}tch_mpoolCb;

static tch_mpoolId tch_mpool_create(size_t sz,uint32_t plen);
static void* tch_mpool_alloc(tch_mpoolId mpool);
static void* tch_mpool_calloc(tch_mpoolId mpool);
static tchStatus tch_mpool_free(tch_mpoolId mpool,void* block);
static tchStatus tch_mpool_destroy(tch_mpoolId mpool);

static void tch_mpoolValidate(tch_mpoolId mp);
static void tch_mpoolInvalidate(tch_mpoolId mp);
static BOOL tch_mpoolIsValid(tch_mpoolId mp);

__attribute__((section(".data"))) static tch_mpool_ix MPoolStaticIntance = {
		tch_mpool_create,
		tch_mpool_alloc,
		tch_mpool_calloc,
		tch_mpool_free,
		tch_mpool_destroy
};

const tch_mpool_ix* Mempool = &MPoolStaticIntance;

tch_mpoolId tch_mpool_create(size_t sz,uint32_t plen){

	tch_mpoolCb* mpcb = (tch_mpoolCb*) tch_shmAlloc(sizeof(tch_mpoolCb) + sz * plen);
	memset(mpcb,0,sizeof(tch_mpoolCb) + sz * plen);
	mpcb->bpool = (tch_mpoolCb*) mpcb + 1;
	mpcb->balign = sz;
	mpcb->__obj.__destr_fn = (tch_kobjDestr) tch_mpool_destroy;
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
	tch_mpoolCb* mpcb = (tch_mpoolCb*) mpool;
	if(!mpcb->bfree)
		return NULL;
	tch_port_atomicBegin();
	void** free = (void**)mpcb->bfree;
	if(free){
		mpcb->bfree = *free;
	}
	tch_port_atomicEnd();
	return free;
}


void* tch_mpool_calloc(tch_mpoolId mpool){
	tch_mpoolCb* mpcb = (tch_mpoolCb*) mpool;
	void* free = tch_mpool_alloc(mpool);
	if(free){
		memset(free,0,mpcb->balign);
	}
	return free;
}

tchStatus tch_mpool_free(tch_mpoolId mpool,void* block){
	tch_mpoolCb* mpcb = (tch_mpoolCb*) mpool;
	if((block < mpcb->bpool) || (block > mpcb->bend))
		return tchErrorParameter;
	tch_port_atomicBegin();
	*((void**)block) = mpcb->bfree;
	mpcb->bfree = block;
	tch_port_atomicEnd();
	return tchOK;
}


static tchStatus tch_mpool_destroy(tch_mpoolId mpool){
	if(!tch_mpoolIsValid(mpool)){
		return tchErrorParameter;
	}
	tch_mpoolInvalidate(mpool);
	tch_mpoolCb* mcb = (tch_mpoolCb*) mpool;
	tch_shmFree(mcb);
	return tchOK;
}


static void tch_mpoolValidate(tch_mpoolId mp){
	((tch_mpoolCb*) mp)->bstate |= (((uint32_t) mp & 0xFFFF) ^ TCH_MPOOL_CLASS_KEY);
}

static void tch_mpoolInvalidate(tch_mpoolId mp){
	((tch_mpoolCb*) mp)->bstate &= ~(0xFFFF);
}

static BOOL tch_mpoolIsValid(tch_mpoolId mp){
	return (((tch_mpoolCb*) mp)->bstate & 0xFFFF) == (((uint32_t) mp & 0xFFFF) ^ TCH_MPOOL_CLASS_KEY);
}

