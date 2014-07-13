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
#include "tch.h"


typedef struct _tch_memp_header {
	void* next;
	size_t len;
}tch_memp_header __attribute__((aligned(4)));

static void* tch_mem_alloc(size_t size);
static int tch_mem_free(void*);

static tch_memp_header* heap_head = NULL;
__attribute__((section(".data")))static tch_mem_ix MEM_StaticInstance = {
		tch_mem_alloc,
		tch_mem_free
};



tch_mem_ix* tch_kernelHeapInit(void* pool,size_t size){
	void* np = (void*)((uint32_t)(pool + 3) & ~3);        /// Make sure pool address is 4 byte aligned address
	size = (size + 3) & ~3;                    /// Make sure size is also 4 byte aligend
	heap_head = (tch_memp_header*) np;       /// Make Head At pool start point
	heap_head->next = (uint8_t*) np + size - sizeof(tch_memp_header);    /// Make Tail At pool end point
	heap_head->len = 0;
	((tch_memp_header*)heap_head->next)->next = NULL;
	((tch_memp_header*)heap_head->next)->len = 0;
	return &MEM_StaticInstance;
}


void* tch_mem_alloc(size_t size){
	void* p = NULL;
	if(!size)
		return NULL;

	tch_memp_header* p_sidx = heap_head;
	tch_memp_header* prev = NULL;
	size = (size + 3) & ~3;                 ///ensure size is 4 byte aligned
	size_t holesz = ((uint8_t*)p_sidx->next - (uint8_t*)p_sidx - p_sidx->len);
	while(size > holesz){
		prev = p_sidx;
		p_sidx = p_sidx->next;
		holesz = ((uint8_t*)p_sidx->next - (uint8_t*)p_sidx - p_sidx->len);
		if(p_sidx == NULL)                  /// reached to end of mem block returns null
			return NULL;
	}
	if(!p_sidx->len){
		p_sidx->len = size;
		return p_sidx + 1;
	}
	p = ((uint8_t*) p_sidx + p_sidx->len + size - sizeof(tch_memp_header));
	((tch_memp_header*) p)->next = p_sidx->next;
	((tch_memp_header*) p)->len = size;
	p_sidx->next = p;
	return (void*)((tch_memp_header*) p + 1);
}

int tch_mem_free(void* fp){
	if(!fp)
		return (1);
	tch_memp_header* s_prev = NULL;
	tch_memp_header* s_idx = heap_head;
	tch_memp_header* mhd = ((tch_memp_header*) fp - 1);
	while(mhd != s_idx){
		s_prev = s_idx;
		s_idx = (tch_memp_header*) s_idx->next;
	}
	if(s_prev == NULL){
		s_idx->len = 0;
	}else{
		s_prev->next = s_idx->next;
	}
	return (0);
}


