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


static void* tch_usrMemAlloc(int size);
static void tch_usrMemFree(void* chnk);


__attribute__((section(".data")))static tch_mem_ix MEM_StaticInstance = {
		tch_usrMemAlloc,
		tch_usrMemFree
};

const tch_mem_ix* Mem = &MEM_StaticInstance;
//__attribute__((section(".data")))static char* heap_end = NULL;





/*
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
*/

tch_memHandle tch_createUsrMem(void* mem,size_t sz){
	/*
	tch_mem_hdr* m_entry = (tch_mem_hdr*) mem++;

	tch_mem_hdr* m_head = (tch_mem_hdr*)(((uint32_t)mem + 7) & ~0x7);
	tch_mem_hdr* m_tail = (tch_mem_hdr*)(((uint32_t)mem + sz) & ~0x7);
	m_tail--;
	m_entry->usz = m_tail->usz = (uint32_t) m_tail - (uint32_t) m_head;
	tch_listInit(&m_entry->allocLn);

	// create first chnk
	m_head->usz = 0;
	tch_listInit((tch_lnode_t*) m_head);
	m_tail->usz = 0;
	tch_listInit((tch_lnode_t*) m_tail);

	tch_listPutFirst((tch_lnode_t*) m_entry,(tch_mem_hdr*) m_head);
	tch_listPutLast((tch_lnode_t*) m_entry,(tch_mem_hdr*) m_tail);

	tch_currentThread->t_mem = (tch_memHandle*) m_entry;*/
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

tchStatus tch_destroyUsrMem(void){
	return osOK;
}


static void* tch_usrMemAlloc(int size){
	/**
	tch_mem_hdr* m_entry = (tch_mem_hdr*) tch_currentThread->t_mem;
	tch_lnode_t* cnode = m_entry;
	tch_mem_hdr* nchnk = NULL;
	int availsz = 0;
	while(cnode->next){
		cnode = cnode->next;
		availsz = ((uint32_t)cnode->next - (uint32_t)cnode) - ((tch_mem_hdr*) cnode)->usz - sizeof(tch_mem_hdr);  // calc available size in current chnk
		if(availsz >= size){
			//allocate it
			nchnk = (tch_mem_hdr*) ((uint32_t)cnode + size);      // create new list node
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
	*/
	tch_mem_hdr* m_entry = (tch_mem_hdr*) tch_currentThread->t_mem;
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

static void tch_usrMemFree(void* p){
	/*
	tch_mem_hdr* m_entry = (tch_mem_hdr*) tch_currentThread->t_mem;
	tch_mem_hdr* chnk = (tch_mem_hdr*) p;
	tch_lnode_t* cnode = NULL;
	chnk--;
	cnode = (tch_lnode_t*) chnk;
	chnk->usz = 0;   // mark as free
	while(!((tch_mem_hdr*)cnode->next)->usz){      // while node is free
		cnode = cnode->next;                 // merge chnk
		((tch_lnode_t*) chnk)->next = cnode->next;
		if(cnode->next)
			cnode->next->prev = chnk;
	}*/

	tch_mem_hdr* m_entry = (tch_mem_hdr*) tch_currentThread->t_mem;
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
