/*
 * tch_condv.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_CONDV_H_
#define TCH_CONDV_H_

#include "tch_Typedef.h"


#if defined(__cplusplus)
extern "C"{
#endif

/***
 *  condition variable types
 */
typedef void* tch_condv_id;

struct _tch_condvar_ix_t {
	tch_condv_id (*create)();
	BOOL (*wait)(tch_condv_id condv,tch_mtxDef* lock,uint32_t timeout);
	tchStatus (*wake)(tch_condv_id condv);
	tchStatus (*wakeAll)(tch_condv_id condv);
	tchStatus (*destroy)(tch_condv_id condv);
};

extern const tch_condv_ix* pcondvar;


#if defined(__cplusplus)
}
#endif
#endif /* TCH_CONDV_H_ */
