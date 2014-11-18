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

static void* tch_memAlloc(tch_memHandle mh,uint32_t size);
static void tch_memFree(tch_memHandle mh,void* p);



static void* tch_usrAlloc(uint32_t size);
static void tch_usrFree(void* chnk);
static void* tch_sharedAlloc(uint32_t size);
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
static void* tch_usrAlloc(uint32_t size){
	return tch_memAlloc(tch_currentThread->t_mem,size);
}

/*!
 * \breif : usr space heap free fucntion
 */
static void tch_usrFree(void* chnk){
	tch_memFree(tch_currentThread->t_mem,chnk);
}

static void* tch_sharedAlloc(uint32_t sz){
	return tch_memAlloc(sharedMem,sz);
}

static void tch_sharedFree(void* chnk){
	tch_memFree(sharedMem,chnk);
}

tch_memHandle tch_memCreate(void* mem,uint32_t sz){
	tch_mem_hdr* m_entry = (tch_mem_hdr*) mem;
	tch_mem_hdr* m_head = (tch_mem_hdr*)((((uint32_t)((tch_mem_hdr*) m_entry + 1)) + 7) & ~7);
	tch_mem_hdr* m_tail = (tch_mem_hdr*) (((uint32_t) mem + sz) & ~7);
	m_tail--;
	m_entry->usz = m_tail->usz = 0;
	m_head->usz = (uint32_t) m_tail - (uint32_t) m_head - sizeof(tch_mem_hdr);
	tch_listInit(&m_entry->allocLn);

	tch_listInit((tch_lnode_t*)m_head);
	tch_listInit((tch_lnode_t*)m_tail);

	tch_listPutFirst((tch_lnode_t*) m_entry,(tch_lnode_t*) m_head);
	tch_listPutLast((tch_lnode_t*) m_entry,(tch_lnode_t*) m_tail);

	return m_entry;

}

tchStatus tch_memDestroy(tch_memHandle mh){
	return osOK;
}

static void* tch_memAlloc(tch_memHandle mh,uint32_t size){
	tch_mem_hdr* m_entry = (tch_mem_hdr*) mh;
	tch_mem_hdr* nchnk = NULL;
	tch_lnode_t* cnode = (tch_lnode_t*)m_entry;
	int rsz = size + sizeof(tch_mem_hdr);
	while(cnode->next){
		cnode = cnode->next;
		if(((tch_mem_hdr*) cnode)->usz > rsz){
			nchnk = (uint32_t) cnode + rsz;
			tch_listInit((tch_lnode_t*) nchnk);
			((tch_lnode_t*)nchnk)->next = cnode->next;
			((tch_lnode_t*)nchnk)->prev = cnode->prev;
			if(cnode->next)
				cnode->next->prev = nchnk;
			if(cnode->prev)
				cnode->prev->next = nchnk;
			nchnk->usz = ((tch_mem_hdr*) cnode)->usz - rsz;
			((tch_mem_hdr*) cnode)->usz = size;
			nchnk = cnode;
			return nchnk + 1;
		}else if(((tch_mem_hdr*) cnode)->usz == size){
			nchnk = cnode;
			if(cnode->next)
				cnode->next->prev = cnode->prev;
			if(cnode->prev)
				cnode->prev->next = cnode->next;
			return nchnk + 1;
		}
	}
	return NULL;
}


static void tch_memFree(tch_memHandle mh,void* p){


	tch_mem_hdr* m_entry = (tch_mem_hdr*) mh;
	tch_mem_hdr* nchnk = p;
	tch_lnode_t* cnode = (tch_lnode_t*)m_entry;
	nchnk--;
	int gap = 0;

	while(cnode->next){
		cnode = cnode->next;
		if((uint32_t) cnode < (uint32_t) nchnk){
			if(((tch_mem_hdr*) cnode)->usz == ((uint32_t)nchnk - (uint32_t)cnode)){
				((tch_mem_hdr*) cnode)->usz += nchnk->usz + sizeof(tch_mem_hdr);
				return;
			}
			((tch_lnode_t*) nchnk)->next = cnode->next;
			((tch_lnode_t*) nchnk)->prev = cnode;
			if(cnode->next)
				cnode->next->prev = nchnk;
			cnode->next = nchnk;
			cnode = nchnk;
			if(nchnk->usz == ((uint32_t) cnode->next - (uint32_t)cnode) - sizeof(tch_mem_hdr)){
				nchnk->usz += ((tch_mem_hdr*) cnode->next)->usz + sizeof(tch_mem_hdr);
				cnode->next = cnode->next->next;
				if(cnode->next)
					cnode->next->prev = cnode;
			}
			return;
		}else{
			((tch_lnode_t*)nchnk)->next = ((tch_lnode_t*)m_entry)->next;
			((tch_lnode_t*)nchnk)->prev = m_entry;
			((tch_lnode_t*)m_entry)->next = nchnk;
			cnode = nchnk;
			if(nchnk->usz == ((uint32_t)cnode->next - (uint32_t)cnode - sizeof(tch_mem_hdr))){
				nchnk->usz += ((tch_mem_hdr*) cnode->next)->usz + sizeof(tch_mem_hdr);
				cnode->next = cnode->next->next;
				if(cnode->next)
					cnode->next->prev = cnode;
			}
			return;
		}
	}
}
