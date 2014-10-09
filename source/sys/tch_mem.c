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
#include "tch_halcfg.h"
#include "tch.h"
#include <stdlib.h>


typedef struct _tch_memp_header {
	void* next;
	size_t len;
}tch_mem_header __attribute__((aligned(4)));

typedef struct _tch_memp_instance {
	tch_mem_handle                  _pix;
	tch_mem_header*             memp_head;
}tch_mem_instance;

static void* tch_apic_mem_alloc(size_t size);
static int tch_apic_free(void* p);

static void* tch_mem_alloc(tch_mem_handle* self,size_t size);
static int tch_mem_free(tch_mem_handle* self,void*);

__attribute__((section(".data")))static tch_mem_ix MEM_StaticInstance = {
#ifndef __USE_MALLOC
		tch_apic_mem_alloc,
		tch_apic_free
#else
		malloc,
		free
#endif

};


const tch_mem_ix* Mem = &MEM_StaticInstance;


tch_mem_handle* tch_memInit(void* pool,size_t size){
	tch_mem_instance* obj = (tch_mem_instance*) pool;
	obj->_pix.alloc = tch_mem_alloc;
	obj->_pix.free = tch_mem_free;
	tch_mem_header** header__p = &obj->memp_head;
	obj++;
	void* np = (void*)((uint32_t)(obj + 3) & ~3);        /// Make sure pool address is 4 byte aligned address
	size = size & ~3;                              /// Make sure size is also 4 byte aligend
	*header__p = (tch_mem_header*) np;                  /// Make Head At pool start point
	(*header__p)->next = (uint8_t*) np + size - sizeof(tch_mem_header);       /// Make Tail At pool end point
	(*header__p)->len = 0;
	((tch_mem_header*)(*header__p)->next)->next = NULL; /// set up list tail so protect from heap request overrun into other memory area
	((tch_mem_header*)(*header__p)->next)->len = 0;
	return (tch_mem_handle*) ((tch_mem_instance*)--obj);
}


void* tch_mem_alloc(tch_mem_handle* self,size_t size){
	void* p = NULL;
	if(!size)
		return NULL;
	tch_mem_instance* obj = (tch_mem_instance*) self;
	tch_mem_header* heap_head = obj->memp_head;
	tch_mem_header* p_sidx = heap_head;
	tch_mem_header* prev = NULL;
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
	p = ((uint8_t*) p_sidx + p_sidx->len + size - sizeof(tch_mem_header));
	((tch_mem_header*) p)->next = p_sidx->next;
	((tch_mem_header*) p)->len = size;
	p_sidx->next = p;
	return (void*)((tch_mem_header*) p + 1);
}

int tch_mem_free(tch_mem_handle* self,void* fp){
	if(!fp)
		return (1);
	tch_mem_instance* obj = (tch_mem_instance*) self;
	tch_mem_header* s_prev = NULL;
	tch_mem_header* s_idx = obj->memp_head;
	tch_mem_header* mhd = ((tch_mem_header*) fp - 1);
	while(mhd != s_idx){
		s_prev = s_idx;
		s_idx = (tch_mem_header*) s_idx->next;
	}
	if(s_prev == NULL){
		s_idx->len = 0;
	}else{
		s_prev->next = s_idx->next;
	}
	return (0);
}

#ifndef __USE_MALLOC
void* tch_apic_mem_alloc(size_t size){
	if(__get_IPSR()){
		return Heap_Manager->alloc(Heap_Manager,size);
	}else{
		return (void*)tch_port_enterSvFromUsr(SV_MEM_MALLOC,size,0);
	}
}

int tch_apic_free(void* p){
	if(__get_IPSR()){
		return Heap_Manager->free(Heap_Manager,p);
	}else{
		return tch_port_enterSvFromUsr(SV_MEM_FREE,(uint32_t)p,0);
	}
}
#endif


