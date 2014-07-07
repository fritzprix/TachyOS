/*
 * tch_mpool.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_MPOOL_H_
#define TCH_MPOOL_H_


/**
 * Macro Function
 */

#define TCH_MPOOL_HEAD_SIZE                    (3 * sizeof(uint32_t) + 3 * sizeof(void*))
/***
 *
 */
#define tch_mpoolDef(name,no,type)\
uint8_t pool_##name[no * sizeof(type) + TCH_MPOOL_HEAD_SIZE]; \
__attribute__((section(".data"))) static tch_mpoolDef_t  mempool_##name = {no,sizeof(type),pool_##name + TCH_MPOOL_HEAD_SIZE}

#define tch_access_mpool(name)\
&mempool_##name


/**
 *  mempool types
 */
typedef struct _tch_mpool_def_t{
	uint32_t count;
	uint32_t align;
	void*    pool;
} tch_mpoolDef_t;


typedef void* tch_mpool_id;

struct _tch_mpool_ix_t {
	tch_mpool_id (*create)(const tch_mpoolDef_t* pool);
	void* (*alloc)(tch_mpool_id mpool);
	void* (*calloc)(tch_mpool_id mpool);
	osStatus (*free)(tch_mpool_id mpool,void* block);
};



#endif /* TCH_MPOOL_H_ */
