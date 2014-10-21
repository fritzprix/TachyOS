/*
 * tch_sem.h
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

#ifndef TCH_SEM_H_
#define TCH_SEM_H_

#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif


/***
 *  semaphore  types
 */
typedef void* tch_semId;

struct _tch_semaph_ix_t {
	tch_semId (*create)(uint32_t count);
	tchStatus (*lock)(tch_semId sid,uint32_t timeout);
	tchStatus (*unlock)(tch_semId sid);
	tchStatus (*destroy)(tch_semId sid);
};


#if defined(__cplusplus)
}
#endif

#endif /* TCH_SEM_H_ */
