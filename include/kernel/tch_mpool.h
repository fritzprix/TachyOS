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


extern tch_mpoolId tch_mpool_create(size_t sz,uint32_t plen);
extern void* tch_mpool_alloc(tch_mpoolId mpool);
extern void* tch_mpool_calloc(tch_mpoolId mpool);
extern tchStatus tch_mpool_free(tch_mpoolId mpool,void* block);
extern tchStatus tch_mpool_destroy(tch_mpoolId mpool);


#if defined(__cplusplus)
}
#endif

#endif /* TCH_MPOOL_H_ */
