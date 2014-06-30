/*
 * tch_mpool.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_MPOOL_H_
#define TCH_MPOOL_H_

/**
 *  mempool types
 */
typedef struct _tch_mpool_def_t tch_mpoolDef_t;
typedef void* tch_mpool_id;

struct _tch_mpool_ix_t {
	tch_mpool_id (*create)(const tch_mpoolDef_t* pool);
	void* (*alloc)(tch_mpool_id mpool);
	void* (*calloc)(tch_mpool_id mpool);
	osStatus (*free)(tch_mpool_id mpool,void* block);
};



#endif /* TCH_MPOOL_H_ */
