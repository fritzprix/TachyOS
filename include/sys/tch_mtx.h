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
#include "tch_thread.h"
#include "tch_TypeDef.h"


#if defined(__cplusplus)
extern "C" {
#endif



/***
 *  mutex  types
 */
typedef void* tch_mtxId;


struct _tch_mutex_ix_t {
	tch_mtxId (*create)();
	tchStatus (*lock)(tch_mtxId mtx,uint32_t timeout);
	tchStatus (*unlock)(tch_mtxId mtx);
	tchStatus (*destroy)(tch_mtxId mtx);
};

#if defined(__cplusplus)
}
#endif


#endif /* TCH_MTX_H_ */
