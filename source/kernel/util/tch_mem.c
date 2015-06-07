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
 *
 *
 * \ brief memory allocator library used in tachyos kernel
 *
 */

#include "tch_mem.h"
#include "tch_kernel.h"
#include "tch_mtx.h"


#define MEM_CLASS_KEY			((uint16_t) 0xF5E2)

#define SET_SAFERETURN();		SAFE_RETURN:
#define RETURN_SAFELY()			goto SAFE_RETURN

#define MEM_VALIDATE(mem)		do {\
	((tch_memEntry*) mem)->status = ((uint32_t) mem ^ MEM_CLASS_KEY) & 0xFFFF;\
}while(0);

#define MEM_INVALIDATE(mem)		do {\
	((tch_memEntry*) mem)->status &= ~0xFFFF;\
}while(0);

#define MEM_ISVALID(mem)		((((tch_memEntry*) mem)->status & 0xFFFF) == (((uint32_t) mem ^ MEM_CLASS_KEY) & 0xFFFF))

typedef struct tch_mem_entry_t tch_memEntry;
typedef struct tch_uobj_prototype tch_uobjProto;
typedef struct tch_mem_chunk_hdr_t tch_memHdr;



struct tch_mem_chunk_hdr_t {
	cdsl_dlistNode_t           allocLn;
	uint32_t              usz;
}__attribute__((aligned(8)));


struct tch_mem_entry_t {
	tch_memHdr          hdr;
	uint32_t 			status;
	tch_mtxId			mtx;
};

struct tch_uobj_prototype {
	tch_memHdr         hdr;
	tch_uobj           __obj;
};

static tch_memHdr* tch_memMerge(tch_memHdr* cur,tch_memHdr* next);

tch_memId tch_memInit(void* mem,uint32_t sz,BOOL isMultiThreaded){
	tch_memEntry* m_entry = (tch_memEntry*) mem;
	if(isMultiThreaded){
		tch_mtxCb* mtxCb = (uint32_t) mem + sizeof(tch_memEntry);		//
		m_entry->mtx = mtxCb;
		tch_mtxInit(mtxCb);
		m_entry = (uint32_t) mtxCb + sizeof(tch_mtxCb);
	}else{
		m_entry->mtx = NULL;
	}

	tch_memHdr* m_head = (tch_memHdr*)((((uint32_t)((tch_memEntry*) m_entry + 1)) + 7) & ~7);
	tch_memHdr* m_tail = (tch_memHdr*) (((uint32_t) mem + sz) & ~7);

	m_entry = (tch_memHdr*) mem;
	m_tail--;
	m_tail->usz = 0;
	m_entry->hdr.usz = m_head->usz = (uint32_t) m_tail - (uint32_t) m_head - sizeof(tch_memHdr);
	cdsl_dlistInit(&m_entry->hdr.allocLn);

	cdsl_dlistInit((cdsl_dlistNode_t*)m_head);
	cdsl_dlistInit((cdsl_dlistNode_t*)m_tail);

	cdsl_dlistPutHead((cdsl_dlistNode_t*) m_entry,(cdsl_dlistNode_t*) m_head);
	cdsl_dlistPutTail((cdsl_dlistNode_t*) m_entry,(cdsl_dlistNode_t*) m_tail);
	MEM_VALIDATE(m_entry);

	return (tch_memId) m_entry;

}

tchStatus tch_memFree(tch_memId mh,void* p,cdsl_dlistNode_t* alc_le){
	if(!mh || !MEM_ISVALID(mh) || !p)
		return tchErrorParameter;
	tch_memEntry* m_entry = (tch_memEntry*) mh;
	tchStatus result = tchOK;
	if(m_entry->mtx){
		if((result = Mtx->lock(m_entry->mtx,tchWaitForever)) != tchOK)
			return result;
	}
	tch_memHdr* nchnk = p;
	cdsl_dlistNode_t* cnode = (cdsl_dlistNode_t*)m_entry;
	nchnk--;
	m_entry->hdr.usz += nchnk->usz + sizeof(tch_memHdr);
	while(cnode->next){
		cnode = cnode->next;
		if(cnode == (cdsl_dlistNode_t*)nchnk){  // if same node is encountered, return immediately
			result = tchErrorParameter;
			RETURN_SAFELY();
		}
		if(((uint32_t) cnode < (uint32_t) nchnk) && ((uint32_t) nchnk < (uint32_t)cnode->next)){
			((cdsl_dlistNode_t*) nchnk)->next = cnode->next;
			((cdsl_dlistNode_t*) nchnk)->prev = cnode;
			if(cnode->next)
				cnode->next->prev = (cdsl_dlistNode_t*) nchnk;
			cnode->next = (cdsl_dlistNode_t*) nchnk;

			do{
				cnode = ((cdsl_dlistNode_t*)nchnk)->next;
				if(!cnode){
					result = tchOK;
					RETURN_SAFELY();
				}
			}while(tch_memMerge(nchnk,(tch_memHdr*)((cdsl_dlistNode_t*)nchnk)->next) != (tch_memHdr*)cnode);
			result = tchOK;
			RETURN_SAFELY();
		}else if(cnode > (cdsl_dlistNode_t*) nchnk){
			((cdsl_dlistNode_t*)nchnk)->next = ((cdsl_dlistNode_t*)m_entry)->next;
			((cdsl_dlistNode_t*)nchnk)->prev = (cdsl_dlistNode_t*)m_entry;
			((cdsl_dlistNode_t*)m_entry)->next = (cdsl_dlistNode_t*)nchnk;
			if(!((cdsl_dlistNode_t*) nchnk)->next){
				result = tchOK;
				RETURN_SAFELY();
			}else{
				((cdsl_dlistNode_t*) nchnk)->next->prev = (cdsl_dlistNode_t*) nchnk;
			}
			do{
				cnode = ((cdsl_dlistNode_t*)nchnk)->next;
				if(!cnode){
					result = tchOK;
					RETURN_SAFELY();
				}
			}while(tch_memMerge(nchnk,(tch_memHdr*)((cdsl_dlistNode_t*)nchnk)->next) != (tch_memHdr*)cnode);
			result = tchOK;
			RETURN_SAFELY();
		}
	}
	m_entry->hdr.usz -= nchnk->usz + sizeof(tch_memHdr);
	result = tchErrorParameter;

	SET_SAFERETURN();
	if(m_entry->mtx){
		Mtx->unlock(m_entry);
	}
	return result;
}

uint32_t tch_memAvail(tch_memId mh){
	if(!mh || !MEM_ISVALID(mh))
		return 0;
	tch_memHdr* m_entry = (tch_memHdr*) mh;
	return m_entry->usz;
}


void* tch_memAlloc(tch_memId mh,size_t size,cdsl_dlistNode_t* alc_list){
	if(!mh || !MEM_ISVALID(mh) || !size)
		return NULL;
	tch_memEntry* m_entry = (tch_memEntry*) mh;
	uint8_t* result = NULL;
	if(m_entry->mtx){
		if(Mtx->lock(m_entry->mtx,tchWaitForever) != tchOK)
			return NULL;
	}
	tch_memHdr* nchnk = NULL;
	cdsl_dlistNode_t* cnode = (cdsl_dlistNode_t*)m_entry;
	int rsz = size + sizeof(tch_memHdr);
	while(cnode->next){
		cnode = cnode->next;
		if(((tch_memHdr*) cnode)->usz > rsz){
			nchnk = (tch_memHdr*)((uint32_t) cnode + rsz);
			cdsl_dlistInit((cdsl_dlistNode_t*) nchnk);
			((cdsl_dlistNode_t*)nchnk)->next = cnode->next;
			((cdsl_dlistNode_t*)nchnk)->prev = cnode->prev;
			if(cnode->next)
				cnode->next->prev = (cdsl_dlistNode_t*)nchnk;
			if(cnode->prev)
				cnode->prev->next = (cdsl_dlistNode_t*)nchnk;
			nchnk->usz = ((tch_memHdr*) cnode)->usz - rsz;
			((tch_memHdr*) cnode)->usz = size;
			nchnk = (tch_memHdr*) cnode;
			m_entry->hdr.usz -= rsz;
			if(alc_list != NULL)
				cdsl_dlistPutTail(alc_list,&nchnk->allocLn);
			nchnk ++;
			result = (uint8_t*)nchnk;
			RETURN_SAFELY();
		}else if(((tch_memHdr*) cnode)->usz == size){
			nchnk = (tch_memHdr*) cnode;
			if(cnode->next)
				cnode->next->prev = cnode->prev;
			if(cnode->prev)
				cnode->prev->next = cnode->next;
			m_entry->hdr.usz -= size + sizeof(tch_memHdr);
			if(alc_list != NULL)
				cdsl_dlistPutTail(alc_list,&nchnk->allocLn);
			nchnk ++;
			result = (uint8_t*)nchnk;
			RETURN_SAFELY();
		}
	}
	result =  NULL;

	SET_SAFERETURN();
	if(m_entry->mtx){
		Mtx->unlock(m_entry->mtx);
	}
	return result;
}

/**
 * \brief merge two adjacent memory chunk, if they are not occupied
 * \param[in] cur memory header of memory chunk to be merged
 * \param[in] next possibly mergable candidate memory chunk with larger address
 * \return if merged successfully return next chunk of the memory header of 'next' parameter
 *
 */
static tch_memHdr* tch_memMerge(tch_memHdr* cur,tch_memHdr* next){
	if(cur->usz == ((uint32_t) next - ((uint32_t) cur) - sizeof(tch_memHdr))){
		cur->usz += next->usz + sizeof(tch_memHdr);
		((cdsl_dlistNode_t*) cur)->next = ((cdsl_dlistNode_t*) next)->next;
		if(((cdsl_dlistNode_t*) cur)->next)
			((cdsl_dlistNode_t*) cur)->next->prev = (cdsl_dlistNode_t*)cur;
		return (tch_memHdr*)((cdsl_dlistNode_t*) cur)->next;
	}
	return next;
}



tchStatus tch_memFreeAll(tch_memId mh,cdsl_dlistNode_t* alloc_list,BOOL exec_destr){
	tch_uobjProto* uobj = NULL;
	tch_memEntry* m_entry = (tch_memEntry*) mh;
	tchStatus result = tchOK;
	if(m_entry->mtx){
		if((result = Mtx->lock(m_entry->mtx,tchWaitForever)) != tchOK)
			return result;
	}

	while(!cdsl_dlistIsEmpty(alloc_list)){
		uobj = cdsl_dlistDequeue(alloc_list);
		if(uobj && uobj->__obj.destructor && exec_destr){
			if(uobj->__obj.destructor(&uobj->__obj) == tchOK){
#ifdef __DBG
				;// print some message
#endif
			}
		}
	}
	if(m_entry->mtx){
		Mtx->unlock(m_entry->mtx);
	}
	return tchOK;
}

