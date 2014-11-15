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

struct tch_mem_chunk_hdr_t {
	tch_lnode_t           allocLn;
	uint32_t              usz;
}__attribute__((aligned(8)));

static void* tch_memAlloc(tch_memHandle mh,int size);
static void tch_memFree(tch_memHandle mh,void* p);



static void* tch_usrAlloc(int size);
static void tch_usrFree(void* chnk);
static void* tch_sharedAlloc(int size);
static void tch_sharedFree(void* chnk);


__attribute__((section(".data")))static tch_mem_ix uMEM_StaticInstance = {
		tch_usrAlloc,
		tch_usrFree
};

__attribute__((section(".data")))static tch_mem_ix kMem_StaticInstance = {
		malloc,
		free
};

__attribute__((section(".data"))) static tch_mem_ix shMem_StaticInstance = {
		tch_sharedAlloc,
		tch_sharedFree
};

const tch_mem_ix* uMem = &uMEM_StaticInstance;
const tch_mem_ix* kMem = &kMem_StaticInstance;
const tch_mem_ix* shMem = &shMem_StaticInstance;
//__attribute__((section(".data")))static char* heap_end = NULL;



/*!
 * \breif : usr space heap allocator function
 */
static void* tch_usrAlloc(int size){
	return tch_memAlloc(tch_currentThread->t_mem,size);
}

/*!
 * \breif : usr space heap free fucntion
 */
static void tch_usrFree(void* chnk){
	tch_memFree(tch_currentThread->t_mem,chnk);
}

static void* tch_sharedAlloc(int size){
	return tch_memAlloc(sharedMem,size);
}

static void tch_sharedFree(void* chnk){
	tch_memFree(sharedMem,chnk);
}


tch_memHandle tch_memCreate(void* mem,size_t sz){
	tch_mem_hdr* m_entry = (tch_mem_hdr*) mem;

	tch_mem_hdr* m_head = (((uint32_t)((tch_mem_hdr*) m_entry + 1)) + 7) & ~7;
	tch_mem_hdr* m_tail = (tch_mem_hdr*)(((uint32_t)mem + sz) & ~0x7);
	m_tail--;
	m_entry->usz = m_tail->usz = (uint32_t) m_tail - (uint32_t) m_head;
	tch_listInit(&m_entry->allocLn);

	// create first chnk
	m_head->usz = 0;
	tch_listInit((tch_lnode_t*) m_head);
	tch_listInit((tch_lnode_t*) m_tail);

	tch_listPutFirst((tch_lnode_t*) m_entry,(tch_mem_hdr*) m_head);
	tch_listPutLast((tch_lnode_t*) m_entry,(tch_mem_hdr*) m_tail);

	return m_entry;

}

tchStatus tch_memDestroy(tch_memHandle mh){
	return osOK;
}


static void* tch_memAlloc(tch_memHandle mh,int size){
	tch_mem_hdr* m_entry = (tch_mem_hdr*) mh;
	tch_lnode_t* cnode = m_entry;
	tch_mem_hdr* nchnk = NULL;
	int availsz = 0;
	while(cnode->next){
		cnode = cnode->next;
		availsz = ((uint32_t)cnode->next - (uint32_t)cnode) - ((tch_mem_hdr*) cnode)->usz - sizeof(tch_mem_hdr);  // calc available size in current chnk
		if(availsz >= size){
			//allocate it
			nchnk = (tch_mem_hdr*) ((uint32_t)cnode + size + sizeof(tch_mem_hdr));      // create new list node
			nchnk->usz = 0;
			((tch_mem_hdr*)cnode)->usz = size;
			((tch_lnode_t*) nchnk)->next = cnode->next;
			cnode->next = (tch_lnode_t*) nchnk;
			((tch_lnode_t*)nchnk)->prev = cnode;
			if(((tch_lnode_t*)nchnk)->next)
				((tch_lnode_t*)nchnk)->next->prev = (tch_lnode_t*) nchnk;
			nchnk = cnode;
			return nchnk + 1;
		}
	}
	return NULL;
}

static void tch_memFree(tch_memHandle mh,void* p){

	tch_mem_hdr* m_entry = (tch_mem_hdr*) mh;
	tch_mem_hdr* chnk = (tch_mem_hdr*) p;
	tch_lnode_t* cnode = NULL;
	chnk--;
	cnode = (tch_lnode_t*) chnk;
	chnk->usz = 0;   // mark as free
	chnk = cnode->next;
	while(!chnk->usz){
		cnode->next = ((tch_lnode_t*) chnk)->next;
		if(cnode->next)
			cnode->next->prev = cnode;
		chnk = ((tch_lnode_t*) chnk)->next;
	}
	if(cnode->prev != m_entry)
		cnode->prev->next = cnode->next;
}
