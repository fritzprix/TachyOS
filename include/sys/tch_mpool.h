/*
 * tch_mpool.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_MPOOL_H_
#define TCH_MPOOL_H_

#include "tch_TypeDef.h"
#include "tch_mtx.h"

#if defined(__cplusplus)
extern "C"{
#endif

/**
 * Macro Function
 */

#define TCH_MPOOL_CB_SIZE                    (3 * sizeof(void*))
/***
 *
 */
#define tch_mpoolDef(name,no,type)\
uint8_t pool_##name[no * sizeof(type) + TCH_MPOOL_CB_SIZE]; \
__attribute__((section(".data"))) static tch_mpoolDef_t  mempool_##name = {no,sizeof(type),pool_##name + TCH_MPOOL_CB_SIZE}

#define tch_access_mpool(name)\
&mempool_##name


/**
 *  mempool types
 */
typedef struct _tch_mpoolDef_t{
	uint32_t count;
	uint32_t align;
	void*    pool;
} tch_mpoolDef_t;


typedef void* tch_mpoolId;

struct _tch_mpool_ix_t {
	tch_mpoolId (*create)(const tch_mpoolDef_t* pool);
	void* (*alloc)(tch_mpoolId mpool);
	void* (*calloc)(tch_mpoolId mpool);
	tchStatus (*free)(tch_mpoolId mpool,void* block);
};

#if defined(__cplusplus)
}
#endif

#endif /* TCH_MPOOL_H_ */
