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

#include "tch_types.h"

#if defined(__cplusplus)
extern "C"{
#endif

extern tch_mpool_ix MPool_IX;
extern tch_mpoolId tch_mpoolCreate(size_t sz,uint32_t plen);
extern void* tch_mpoolAlloc(tch_mpoolId mpool);
extern void* tch_mpoolCalloc(tch_mpoolId mpool);
extern tchStatus tch_mpoolFree(tch_mpoolId mpool,void* block);
extern tchStatus tch_mpoolDestroy(tch_mpoolId mpool);


#if defined(__cplusplus)
}
#endif

#endif /* TCH_MPOOL_H_ */
