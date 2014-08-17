/*
 * tch_mtx.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 30.
 *      Author: innocentevil
 */

#ifndef TCH_MTX_H_
#define TCH_MTX_H_

#include "tch_list.h"

#define MTX_INIT_MARK                 ((uint32_t) 0x01)
#define INIT_MTX                      {MTX_INIT_MARK,{INIT_LIST}}

/***
 *  mutex  types
 */
typedef struct _tch_mtx_t tch_mtx;
typedef struct _tch_mtx_waitque_t {
	tch_lnode_t          que;
}tch_mtx_waitque;

struct _tch_mtx_t {
	uint32_t            key;
	tch_mtx_waitque     que;
};

struct _tch_mutex_ix_t {
	tch_mtx* (*create)(tch_mtx* mtx);
	tchStatus (*lock)(tch_mtx* mtx,uint32_t timeout);
	tchStatus (*unlock)(tch_mtx* mtx);
	tchStatus (*destroy)(tch_mtx* mtx);
};


#endif /* TCH_MTX_H_ */
