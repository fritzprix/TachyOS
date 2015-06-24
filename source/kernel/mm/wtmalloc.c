/*
 * wtmalloc.c
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 */

#include "wtree.h"
#include "wtmalloc.h"


#ifndef container_of
#define container_of(ptr,type,member) 		 (((size_t) ptr - (size_t) offsetof(type,member)))
#endif
struct exter_header {
	size_t psize;				///			prev chunk			///
/// ===========================================================	///
	size_t size;				///         current chunk
/// ===========================================================	///
};
struct heapHeader {
	size_t psize;				///			prev chunk			///
/// ===========================================================	///
	size_t size;				///         current chunk
/// ===========================================================	///
	wtreeNode_t wtree_node;		///			free cache header 	///
};

void wtreeHeap_init(wt_alloc_t* alloc,void* addr,size_t sz){
	alloc->heap_base = alloc->heap_pos = (uint8_t*) addr;
	alloc->heap_limit = (size_t) addr + sz;
	alloc->heap_size = sz;
	wtreeRootInit(&alloc->cache_root,sizeof(struct exter_header));
}

void * wtreeHeap_malloc(wt_alloc_t* alloc,size_t sz){
	if(!sz)
		return NULL;
	wtreeNode_t* chunk = wtreeRetrive(&alloc->cache_root,&sz);
	struct heapHeader *chdr,*nhdr,*nnhdr;
	uint64_t tsz;
	if(chunk == NULL){
		if(alloc->heap_pos + sz + sizeof(struct heapHeader) >= alloc->heap_limit)
			return NULL; // impossible to handle
		chdr = (struct heapHeader*) alloc->heap_pos;
		alloc->heap_pos = (size_t) &chdr->wtree_node + sz;
		nhdr = (struct heapHeader*) alloc->heap_pos;
		chdr->size = sz;
		nhdr->psize = sz;
		return &chdr->wtree_node;
	}else{
		chdr = container_of(chunk,struct heapHeader,wtree_node);  //?
		nnhdr = (struct heapHeader *) ((size_t) &chdr->wtree_node + sz);
		chdr->size = sz;
		nnhdr->psize = sz;
		return &chdr->wtree_node;
	}
}

void wtreeHeap_free(wt_alloc_t* alloc,void* ptr){
	if(!ptr)
		return;
	struct heapHeader* chdr,*nhdr;
	chdr = container_of(ptr,struct heapHeader,wtree_node);
	nhdr = (struct heapHeader*) ((size_t)ptr + chdr->size);
	if(chdr->size != nhdr->psize)
		error(-1,0,"Heap Corrupted\n");
	wtreeNodeInit(&chdr->wtree_node,&chdr->wtree_node,chdr->size);
	wtreeInsert(&alloc->cache_root,&chdr->wtree_node);
}

size_t wtreeHeap_size(wt_alloc_t* alloc){
	return wtreeTotalSpan(&alloc->cache_root);
}


void wtreeHeap_print(wt_alloc_t* alloc){
	printf("======================================================================================================\n");
	wtreePrint(&alloc->cache_root);
	printf("======================================================================================================\n");
}

