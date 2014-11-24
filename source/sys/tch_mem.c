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

typedef struct tch_mem_entry_t tch_memEntry;
typedef struct tch_uobj_prototype tch_uobjProto;
typedef struct tch_mem_chunk_hdr_t tch_memHdr;

struct tch_mem_chunk_hdr_t {
	tch_lnode_t           allocLn;
	uint32_t              usz;
}__attribute__((aligned(8)));




struct tch_mem_entry_t {
	tch_memHdr            hdr;
	tch_mtxId             mtx;
};

struct tch_uobj_prototype {
	tch_memHdr         hdr;
	tch_uobj            __obj;
};

static tch_memEntry* tch_memCreate(void* mem,uint32_t sz);
static void* tch_memAlloc(tch_memHandle mh,size_t size);
static tch_memHdr* tch_memMerge(tch_memHdr* cur,tch_memHdr* next);
static void tch_memFree(tch_memHandle mh,void* p);
static uint32_t tch_memAvail(tch_memHandle mh);
static tchStatus tch_memFreeAll(tch_memHandle mh,tch_lnode_t* alloc_list);
static void tch_memPrint(void*);


static void* tch_usrAlloc(size_t size);
static void tch_usrFree(void* chnk);
static uint32_t tch_usrAvail(void);
static tchStatus tch_usrFreeAll(tch_threadId thread);
static void tch_usrPrintFreeList(void);
static void tch_usrPrintAllocList(void);

static void* tch_sharedAlloc(size_t size);
static void tch_sharedFree(void* chnk);
static uint32_t tch_sharedAvail(void);
static tchStatus tch_sharedFreeAll(tch_threadId thread);
static void tch_sharedPrintFreeList(void);
static void tch_sharedPrintAllocList(void);


__attribute__((section(".data")))static tch_mem_ix uMEM_StaticInstance = {
		tch_usrAlloc,
		tch_usrFree,
		tch_usrAvail,
		tch_usrFreeAll,
		tch_usrPrintFreeList,
		tch_usrPrintAllocList
};

__attribute__((section(".data")))static tch_mem_ix kMem_StaticInstance = {
		malloc,
		free,
		tch_kHeapAvail,
		tch_kHeapFreeAll,
		NULL,
		NULL
};

__attribute__((section(".data"))) static tch_mem_ix shMem_StaticInstance = {
		tch_sharedAlloc,
		tch_sharedFree,
		tch_sharedAvail,
		tch_sharedFreeAll,
		tch_sharedPrintFreeList,
		tch_sharedPrintAllocList
};

const tch_mem_ix* uMem = &uMEM_StaticInstance;      // dynamic memory block which can be used by user process (threads) and accessible only by owner process and its child thread
const tch_mem_ix* kMem = &kMem_StaticInstance;      // dynamic memory block which is soley used by kernel function and system library (will be protected from user thread)
const tch_mem_ix* shMem = &shMem_StaticInstance;    // dynamic memory block which can be shared by multiple threads (for IPC ..)


tch_memHandle tch_usrMemCreate(void* mem,uint32_t sz){
	tch_memEntry* m_entry = tch_memCreate(mem,sz);
//	m_entry->mtx = Mtx->create();
	return (tch_memHandle)m_entry;
}

tch_memHandle tch_sharedMemCreate(void* mem,uint32_t sz){
	tch_memEntry* _entry = tch_memCreate(mem,sz);
	sharedMem = (tch_memHandle)_entry;
	_entry->mtx = Mtx->create();
	return (tch_memHandle) _entry;
}

/*!
 * \breif : usr space heap allocator function
 */
static void* tch_usrAlloc(size_t size){
	if(tch_port_isISR())
		return NULL;
	tch_memHdr* hdr = tch_memAlloc(tch_currentThread->t_mem,size);
	if(!hdr)
		Thread->terminate(tch_currentThread,osErrorNoMemory);
	hdr--;
	tch_listPutLast(&tch_currentThread->t_ualc,(tch_lnode_t*)hdr);
	return ++hdr;
}

/*!
 * \breif : usr space heap free fucntion
 */
static void tch_usrFree(void* chnk){
	if(tch_port_isISR())
		return;
	tch_memHdr* hdr = chnk;
	hdr--;
	tch_listRemove(&tch_currentThread->t_ualc,(tch_lnode_t*)hdr);
	tch_memFree(tch_currentThread->t_mem,chnk);
}

static uint32_t tch_usrAvail(void){
	return tch_memAvail(tch_currentThread->t_mem);
}

static tchStatus tch_usrFreeAll(tch_threadId thread){
	tch_thread_header* th_hdr = (tch_thread_header*) thread;
	return tch_memFreeAll(th_hdr->t_mem,&th_hdr->t_ualc);
}

static void tch_usrPrintFreeList(void){
	tch_listPrint(tch_currentThread->t_mem,tch_memPrint);
}

static void tch_usrPrintAllocList(void){
	tch_listPrint(&tch_currentThread->t_ualc,tch_memPrint);
}


void* tch_sharedAlloc(size_t sz){
	tch_memEntry* _entry = (tch_memEntry*) sharedMem;
	if(tch_port_isISR())
		return NULL;
	if(_entry->mtx){
		if(Mtx->lock(_entry->mtx,osWaitForever) != osOK)
			return NULL;
	}

	tch_memHdr* hdr = tch_memAlloc(sharedMem,sz);

	if(!hdr){
		if(_entry->mtx){
			Mtx->unlock(_entry->mtx);
		}
		Thread->terminate(tch_currentThread,osErrorNoMemory);
	}
	hdr--;
	tch_listPutLast(&tch_currentThread->t_shalc,(tch_lnode_t*)hdr);
	if(_entry->mtx){
		Mtx->unlock(_entry->mtx);
	}
	return ++hdr;
}

void tch_sharedFree(void* chnk){
	tch_memEntry* m_entry = (tch_memEntry*) sharedMem;
	if(tch_port_isISR())
		return;
	tch_memHdr* hdr = chnk;
	hdr--;
	if(Mtx->lock(m_entry->mtx,osWaitForever) != osOK)
		return;
	tch_listRemove(&tch_currentThread->t_shalc,(tch_lnode_t*)hdr);
	tch_memFree(sharedMem,chnk);
	Mtx->unlock(m_entry->mtx);
}

static uint32_t tch_sharedAvail(void){
	return tch_memAvail(sharedMem);
}

static tchStatus tch_sharedFreeAll(tch_threadId thread){
	tch_thread_header* th_hdr = (tch_thread_header*) thread;
	return tch_memFreeAll(sharedMem,&th_hdr->t_shalc);
}

static void tch_sharedPrintFreeList(void){
	tch_listPrint(sharedMem,tch_memPrint);
}

static void tch_sharedPrintAllocList(void){
	tch_listPrint(&tch_currentThread->t_shalc,tch_memPrint);
}


static tch_memEntry* tch_memCreate(void* mem,uint32_t sz){
	tch_memEntry* m_entry = (tch_memEntry*) mem;
	tch_memHdr* m_head = (tch_memHdr*)((((uint32_t)((tch_memEntry*) m_entry + 1)) + 7) & ~7);
	tch_memHdr* m_tail = (tch_memHdr*) (((uint32_t) mem + sz) & ~7);
	m_tail--;
	m_tail->usz = 0;
	m_entry->hdr.usz = m_head->usz = (uint32_t) m_tail - (uint32_t) m_head - sizeof(tch_memHdr);
	tch_listInit(&m_entry->hdr.allocLn);

	tch_listInit((tch_lnode_t*)m_head);
	tch_listInit((tch_lnode_t*)m_tail);

	tch_listPutFirst((tch_lnode_t*) m_entry,(tch_lnode_t*) m_head);
	tch_listPutLast((tch_lnode_t*) m_entry,(tch_lnode_t*) m_tail);

	return m_entry;

}

static tchStatus tch_memFreeAll(tch_memHandle mh,tch_lnode_t* alloc_list){
	tch_uobjProto* uobj = NULL;
	while(!tch_listIsEmpty(alloc_list)){
		uobj = tch_listDequeue(alloc_list);
		if(uobj && uobj->__obj.destructor){
			if(uobj->__obj.destructor(&uobj->__obj) == osOK)
				uStdLib->stdio->iprintf("\r leaked resources closed\n");
		}
	}
	return osOK;
}

static void tch_memPrint(void* m){
	uStdLib->stdio->iprintf("\rChunk : sizeof %d / addr %d\n",((tch_memHdr*)m)->usz,m);
}


static void* tch_memAlloc(tch_memHandle mh,size_t size){
	tch_memEntry* m_entry = (tch_memEntry*) mh;
	tch_memHdr* nchnk = NULL;
	tch_lnode_t* cnode = (tch_lnode_t*)m_entry;
	int rsz = size + sizeof(tch_memHdr);
	while(cnode->next){
		cnode = cnode->next;
		if(((tch_memHdr*) cnode)->usz > rsz){
			nchnk = (tch_memHdr*)((uint32_t) cnode + rsz);
			tch_listInit((tch_lnode_t*) nchnk);
			((tch_lnode_t*)nchnk)->next = cnode->next;
			((tch_lnode_t*)nchnk)->prev = cnode->prev;
			if(cnode->next)
				cnode->next->prev = (tch_lnode_t*)nchnk;
			if(cnode->prev)
				cnode->prev->next = (tch_lnode_t*)nchnk;
			nchnk->usz = ((tch_memHdr*) cnode)->usz - rsz;
			((tch_memHdr*) cnode)->usz = size;
			nchnk = (tch_memHdr*) cnode;
			m_entry->hdr.usz -= rsz;
			return nchnk + 1;
		}else if(((tch_memHdr*) cnode)->usz == size){
			nchnk = (tch_memHdr*) cnode;
			if(cnode->next)
				cnode->next->prev = cnode->prev;
			if(cnode->prev)
				cnode->prev->next = cnode->next;
			m_entry->hdr.usz -= size + sizeof(tch_memHdr);
			return nchnk + 1;
		}
	}
	return NULL;
}

static tch_memHdr* tch_memMerge(tch_memHdr* cur,tch_memHdr* next){
	if(cur->usz == ((uint32_t) next - ((uint32_t) cur) - sizeof(tch_memHdr))){
		cur->usz += next->usz + sizeof(tch_memHdr);
		((tch_lnode_t*) cur)->next = ((tch_lnode_t*) next)->next;
		if(((tch_lnode_t*) cur)->next)
			((tch_lnode_t*) cur)->next->prev = (tch_lnode_t*)cur;
		return (tch_memHdr*)((tch_lnode_t*) cur)->next;
	}
	return next;
}


static void tch_memFree(tch_memHandle mh,void* p){
	tch_memEntry* m_entry = (tch_memEntry*) mh;
	tch_memHdr* nchnk = p;
	tch_lnode_t* cnode = (tch_lnode_t*)m_entry;
	nchnk--;
	m_entry->hdr.usz += nchnk->usz + sizeof(tch_memHdr);
	while(cnode->next){
		cnode = cnode->next;
		if(cnode == (tch_lnode_t*)nchnk){  // if same node is encountered, return immediately
			return;
		}
		if(((uint32_t) cnode < (uint32_t) nchnk) && ((uint32_t) nchnk < (uint32_t)cnode->next)){
			((tch_lnode_t*) nchnk)->next = cnode->next;
			((tch_lnode_t*) nchnk)->prev = cnode;
			if(cnode->next)
				cnode->next->prev = (tch_lnode_t*) nchnk;
			cnode->next = (tch_lnode_t*) nchnk;

			do{
				cnode = ((tch_lnode_t*)nchnk)->next;
				if(!cnode){
					return;
				}
			}while(tch_memMerge(nchnk,(tch_memHdr*)((tch_lnode_t*)nchnk)->next) != (tch_memHdr*)cnode);
			return;
		}else if(cnode > (tch_lnode_t*) nchnk){
			((tch_lnode_t*)nchnk)->next = ((tch_lnode_t*)m_entry)->next;
			((tch_lnode_t*)nchnk)->prev = (tch_lnode_t*)m_entry;
			((tch_lnode_t*)m_entry)->next = (tch_lnode_t*)nchnk;

			if(!((tch_lnode_t*) nchnk)->next){
				return;
			}
			do{
				cnode = ((tch_lnode_t*)nchnk)->next;
				if(!cnode){
					return;
				}
			}while(tch_memMerge(nchnk,(tch_memHdr*)((tch_lnode_t*)nchnk)->next) != (tch_memHdr*)cnode);
			return;
		}
	}
	m_entry->hdr.usz -= nchnk->usz + sizeof(tch_memHdr);
}


static uint32_t tch_memAvail(tch_memHandle mh){
	tch_memHdr* m_entry = (tch_memHdr*) mh;
	return m_entry->usz;
}
