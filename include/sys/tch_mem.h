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

#include <stddef.h>
typedef struct _tch_mem_handle tch_mem_handle;

/**
 * @brief This interface is NOT available in API (Public)
 *        Only used in kernel internal program
 */
struct _tch_mem_handle {
	void* (*alloc)(tch_mem_handle* handle,size_t size);
	int (*free)(tch_mem_handle* handle,void*);
};


/**
 * @brief This is public API for Dynamic Memory Allocation
 */
struct _tch_mem_ix_t {
	void* (*alloc)(size_t size);
	int (*free)(void*);
};

extern tch_mem_handle* tch_memInit(void* pool,size_t size);

#endif /* TCH_MEM_H_ */
