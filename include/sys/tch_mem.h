/*
 * tch_mem.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */

#ifndef TCH_MEM_H_
#define TCH_MEM_H_


#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif

typedef void* tch_memHandle;



/**
 * @brief This is public API for Dynamic Memory Allocation
 */
struct _tch_mem_ix_t {
	void* (*alloc)(uint32_t size);
	void (*free)(void*);
};

tch_memHandle tch_memCreate(void* mem,uint32_t sz);
tchStatus tch_memDestroy(tch_memHandle);

#if defined(__cplusplus)
}
#endif
#endif /* TCH_MEM_H_ */
