/*
 * tch_mtx.h
 *
 *  Created on: 2014. 6. 30.
 *      Author: innocentevil
 */

#ifndef TCH_MTX_H_
#define TCH_MTX_H_

#include "lib/tch_absdata.h"

#define MTX_INIT_MARK                 ((uint32_t) 0x01)
#define getMtxObject(id)              ((tch_mtx*) id)

/***
 *  mutex  types
 */
typedef void* tch_mtx_id;
typedef struct _tch_mtx_t tch_mtx;
typedef struct _tch_mtx_waitque_t {
	tch_genericList_queue_t que;
}tch_mtx_waitque;


struct _tch_mtx_t {
	uint32_t            key;
	tch_mtx_waitque     que;
};

struct _tch_mutex_ix_t {
	tch_mtx_id (*create)(tch_mtx* mtx);
	osStatus (*lock)(tch_mtx_id mtx,uint32_t timeout);
	osStatus (*unlock)(tch_mtx_id mtx);
	osStatus (*destroy)(tch_mtx_id mtx);
};


#endif /* TCH_MTX_H_ */
