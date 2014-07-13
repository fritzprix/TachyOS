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

struct _tch_mem_ix_t {
	void* (*alloc)(size_t size);
	int (*free)(void*);
};

#endif /* TCH_MEM_H_ */
