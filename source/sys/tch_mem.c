/*
 * tch_mem.c
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */
#include "tch_kernel.h"
#include "tch_sys.h"
#include "tch_halcfg.h"
#include "tch.h"
#include <stdlib.h>


__attribute__((section(".data")))static tch_mem_ix MEM_StaticInstance = {
		malloc,
		free
};

const tch_mem_ix* Mem = &MEM_StaticInstance;
__attribute__((section(".data")))static char* heap_end = NULL;


void* tch_sbrk_k(struct _reent* reent,size_t incr){
	if(heap_end == NULL)
		heap_end = (char*)&Heap_Base;
	char *prev_heap_end;
	prev_heap_end = heap_end;
	if ((uint32_t)heap_end + incr > (uint32_t) &Heap_Limit) {
		return NULL;
	}
	heap_end += incr;
	return prev_heap_end;
}

