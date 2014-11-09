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


typedef struct tch_mem_chunk_hdr_t tch_mem_hdr;
typedef struct tch_mem_prototype_t tch_mem_prototype;

struct tch_mem_chunk_hdr_t {
	tch_lnode_t           allocLn;
	uint32_t              size;
}__attribute__((aligned(8)));

typedef struct tch_mem_prototype_t {
	tch_mem_ix           _pix;
	tch_mem_hdr           hdr_entry;
}tch_mem_prototype;

__attribute__((section(".data")))static tch_mem_ix MEM_StaticInstance = {
		malloc,
		free
};

const tch_mem_ix* Mem = &MEM_StaticInstance;
__attribute__((section(".data")))static char* heap_end = NULL;

static void* tch_usrMemAlloc(tch_mem_ix* mem,uint32_t size);
static void tch_usrMemFree(tch_mem_ix* mem,void* chnk);


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

tch_mem_ix* tch_createUsrMem(void* mem,size_t sz){
	tch_mem_prototype* mhdr = (tch_mem_prototype*) mem++;
	uint8_t* mb = ((uint32_t)(mem + 7) & ~0x7);
	uint8_t* mt = ((uint32_t) mem + sz) & ~(1 << 7);
	mhdr->_pix.alloc = tch_usrMemAlloc;
	mhdr->_pix.free = tch_usrMemFree;
	tch_listInit(&mhdr->hdr_entry.allocLn);
	mhdr->hdr_entry.size = (uint32_t) (mt - mb);

	// create first chnk
	((tch_mem_hdr*) mb)->size = 0;
	tch_listInit(&((tch_mem_hdr*) mb)->allocLn);
	((tch_mem_hdr*) mt - 1)->size = 0;
	tch_listInit(&((tch_mem_hdr*) mt - 1)->allocLn);

	tch_listPush(&mhdr->hdr_entry,&((tch_mem_hdr*) mt - 1)->allocLn);
	tch_listPush(&mhdr->hdr_entry,&((tch_mem_hdr*) mb)->allocLn);

	return (tch_mem_ix*) mhdr;
}

void* tch_destroyUsrMem(tch_mem_ix* mem){
	return (tch_mem_prototype*) --mem;
}


static void* tch_usrMemAlloc(tch_mem_ix* mem,uint32_t size){
	tch_mem_prototype* mhdr = (tch_mem_prototype*) mem;
	tch_lnode_t* alloclist = &mhdr->hdr_entry;
	tch_mem_hdr* nchnk = NULL;
	uint32_t availsz = 0;
	while(alloclist->next){
		alloclist = alloclist->next;
		availsz = ((uint32_t)alloclist->next - (uint32_t)alloclist) - ((tch_mem_hdr*) alloclist)->size - sizeof(tch_mem_hdr);  // calc available size in current chnk
		if(availsz >= size){
			//allocate it
			nchnk = (tch_mem_hdr*) ((uint32_t)alloclist + ((tch_mem_hdr*)alloclist)->size);
			nchnk->size = size;
			((tch_lnode_t*) nchnk)->next = alloclist->next;
			alloclist->next = (tch_lnode_t*) nchnk;
			nchnk->allocLn.prev = alloclist;
			if(nchnk->allocLn.next)
				nchnk->allocLn.next->prev = (tch_lnode_t*) nchnk;
			return (tch_mem_hdr*)++nchnk;
		}
	}
	return NULL;
}

static void tch_usrMemFree(tch_mem_ix* mem,void* chnk){
	tch_mem_prototype* mhdr = (tch_mem_prototype*) mem;
	tch_mem_hdr* chnk_hdr = (tch_mem_hdr*) --mem;
	tch_mem_hdr* prev_chnk_hdr = NULL;
	tch_lnode_t* allocList = &mhdr->hdr_entry;
	while(allocList->next){
		allocList = allocList->next;
		if((uint32_t)chnk_hdr == (uint32_t)allocList){
			prev_chnk_hdr = (tch_mem_hdr*)chnk_hdr->allocLn.prev;
			prev_chnk_hdr->size += chnk_hdr->size + sizeof(tch_mem_hdr);
			prev_chnk_hdr->allocLn.next = chnk_hdr->allocLn.next;
			if(prev_chnk_hdr->allocLn.next)
				prev_chnk_hdr->allocLn.next->prev = prev_chnk_hdr;
			return;
		}
	}
}
