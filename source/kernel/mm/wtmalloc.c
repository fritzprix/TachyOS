/*
 * wtmalloc.c
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 */

#include "wtree.h"
#include "wtmalloc.h"
#include "tch_kernel.h"



#ifndef container_of
#define container_of(ptr,type,member) 		 (((size_t) ptr - (size_t) offsetof(type,member)))
#endif

#define NULL_CACHE 					&null_node

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



static void * cache_malloc(wt_alloc_t* alloc,size_t sz);
static size_t cache_free(wt_alloc_t* alloc,void* ptr);
static size_t cache_size(wt_alloc_t* alloc);
static void cache_print(wt_alloc_t* alloc);

static wt_alloc_t* rotateLeft(wt_alloc_t* rot_pivot);
static wt_alloc_t* rotateRight(wt_alloc_t* rot_pivot);
static wt_alloc_t* add_cache_r(wt_alloc_t* current,wt_alloc_t* nu);
static wt_alloc_t* free_cache_r(wt_alloc_t* current,void* ptr,size_t *sz);
static size_t available_r(wt_alloc_t* current);
static void print_tab(int k);
static void print_r(wt_alloc_t* current,int k);


/*
 * 	wt_alloc_t *left,*right;
	void*	 	base;
	void* 		pos;
	void* 		limit;
	size_t 		size;
	wtreeRoot_t	entry;
 */
static wt_alloc_t null_node = {
		.left = NULL,
		.right = NULL,
		.pos = NULL,
		.limit = NULL,
		.size = 0,
		.entry = {0}
};

void wtreeHeap_initCacheRoot(wt_heaproot_t* root){
	if(!root)
		return;
	root->cache = NULL_CACHE;
	root->size = 0;
	root->free_sz = 0;
}

void wtreeHeap_initCacheNode(wt_alloc_t* alloc,void* addr,size_t sz){
	alloc->base = alloc->pos = (uint8_t*) addr;
	alloc->limit = (void*) ((size_t) addr + sz);
	alloc->size = sz;
	alloc->left = alloc->right = NULL_CACHE;
	wtreeRootInit(&alloc->entry,sizeof(struct exter_header));
}


void wtreeHeap_addCache(wt_heaproot_t* heap,wt_alloc_t* cache){
	if(!heap || !cache_free)
		return;
	heap->cache = add_cache_r(heap->cache,cache);
	heap->size += cache->size;
	heap->free_sz += cache->size;
}

void* wtreeHeap_malloc(wt_heaproot_t* heap,size_t sz){
	if(!heap || !sz)
		return NULL;
	void* chunk_ptr = cache_malloc(heap->cache,sz);
	if((heap->cache->left->size > heap->cache->size) || (heap->cache->right->size > heap->cache->size)){
		if(heap->cache->left->size > heap->cache->right->size){
			heap->cache = rotateRight(heap->cache);
		}else{
			heap->cache = rotateLeft(heap->cache);
		}
	}
	heap->free_sz -= sz;
	return chunk_ptr;
}

void wtreeHeap_free(wt_heaproot_t* heap,void* ptr){
	if(!heap || !ptr)
		return;
	if(heap->cache == NULL_CACHE)
		exit(-1);
	size_t sz = 0;
	heap->cache = free_cache_r(heap->cache,ptr,&sz);
	if(sz != 0)
		heap->free_sz += sz;
}

size_t wtreeHeap_size(wt_heaproot_t* heap){
	if(!heap)
		return 0;
	return heap->size;
}

size_t wtreeHeap_available(wt_heaproot_t* heap){
	if(!heap)
		return 0;
	return heap->free_sz;
}


void wtreeHeap_print(wt_heaproot_t* heap){
	if(!heap)
		return;
	print_r(heap->cache,0);
}


static wt_alloc_t* add_cache_r(wt_alloc_t* current,wt_alloc_t* nu){
	if(current == NULL_CACHE)
		return nu;
	if(current->base < nu->base){
		current->right = add_cache_r(current->right,nu);
		if(current->size < current->right->size)
			return rotateLeft(current);
	}else{
		current->left = add_cache_r(current->left,nu);
		if(current->size < current->left->size)
			return rotateRight(current);
	}
	return current;
}


/**
 *
 */
static wt_alloc_t* free_cache_r(wt_alloc_t* current,void* ptr,size_t *sz){
	if(current->base > ptr){
		current->left = free_cache_r(current->left,ptr,sz);
		if(current->left->size > current->size)
			return rotateRight(current);

	}
	else if((current->base <= ptr) && (current->limit > ptr)) {
		*sz = cache_free(current,ptr);
	} else {
		current->right = free_cache_r(current->right,ptr,sz);
		if(current->right->size > current->size)
			return rotateLeft(current);
	}
	return current;
}

static size_t available_r(wt_alloc_t* current){
	if(current == NULL_CACHE)
		return 0;
	return available_r(current->left) + current->size + available_r(current->right);
}


static void print_tab(int k){
	while(k--)	printf("\t");
}


static void print_r(wt_alloc_t* current,int k){
	if(current == NULL_CACHE)
		return;
	print_r(current->left,k + 1);
	print_tab(k);printf("{cache : base %d , size %d @ %d}\n",current->base,current->size,k);
	print_r(current->right,k + 1);
}




static wt_alloc_t* rotateLeft(wt_alloc_t* rot_pivot){
	wt_alloc_t* nparent = rot_pivot->right;
	rot_pivot->right = nparent->left;
	nparent->left = rot_pivot;
	return nparent;
}

static wt_alloc_t* rotateRight(wt_alloc_t* rot_pivot){
	wt_alloc_t* nparent = rot_pivot->left;
	rot_pivot->left = nparent->right;
	nparent->right = rot_pivot;
	return nparent;
}

static void * cache_malloc(wt_alloc_t* alloc,size_t sz){
	if(!sz)
		return NULL;
	wtreeNode_t* chunk = wtreeRetrive(&alloc->entry,&sz);
	struct heapHeader *chdr,*nhdr,*nnhdr;
	uint64_t tsz;
	if(chunk == NULL){
		if(alloc->pos + sz + sizeof(struct heapHeader) >= alloc->limit)
			return NULL; // impossible to handle
		chdr = (struct heapHeader*) alloc->pos;
		alloc->pos = (void*) ((size_t) &chdr->wtree_node + sz);
		nhdr = (struct heapHeader*) alloc->pos;
		chdr->size = sz;
		nhdr->psize = sz;
	}else{
		chdr = (struct heapHeader *) container_of(chunk,struct heapHeader,wtree_node);  //?
		nnhdr = (struct heapHeader *) ((size_t) &chdr->wtree_node + sz);
		chdr->size = sz;
		nnhdr->psize = sz;
	}
	alloc->size -= sz;
	return &chdr->wtree_node;
}

static size_t cache_free(wt_alloc_t* alloc,void* ptr){
	if(!ptr)
		return 0;
	struct heapHeader* chdr,*nhdr;
	chdr = (struct heapHeader*) container_of(ptr,struct heapHeader,wtree_node);
	nhdr = (struct heapHeader*) ((size_t)ptr + chdr->size);
	if(chdr->size != nhdr->psize)
		Thread->terminate(tch_currentThread,tchErrorNoMemory);
	wtreeNodeInit(&chdr->wtree_node,(uint32_t)&chdr->wtree_node,chdr->size);
	wtreeInsert(&alloc->entry,&chdr->wtree_node);
	alloc->size += chdr->size;
	return chdr->size;
}

static size_t cache_size(wt_alloc_t* alloc){
	return wtreeTotalSpan(&alloc->entry);
}


static void cache_print(wt_alloc_t* alloc){
	printf("======================================================================================================\n");
	wtreePrint(&alloc->entry);
	printf("======================================================================================================\n");
}


