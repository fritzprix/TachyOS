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
#define container_of(ptr,type,member) 		 (((uint32_t) ptr - (uint32_t) offsetof(type,member)))
#endif

#define NULL_CACHE 					&null_node

struct ext_header {
	uint32_t psize;				///			prev chunk			///
/// ===========================================================	///
	uint32_t size;				///         current chunk
/// ===========================================================	///
};
struct heapHeader {
	uint32_t psize;				///			prev chunk			///
/// ===========================================================	///
	uint32_t size;				///         current chunk
/// ===========================================================	///
	wtreeNode_t wtree_node;		///			free cache header 	///
};



static void * node_malloc(wt_heapNode_t* alloc,uint32_t sz);
static uint32_t node_free(wt_heapNode_t* alloc,void* ptr);
static uint32_t node_size(wt_heapNode_t* alloc);
static void node_print(wt_heapNode_t* alloc);

static wt_heapNode_t* rotateLeft(wt_heapNode_t* rot_pivot);
static wt_heapNode_t* rotateRight(wt_heapNode_t* rot_pivot);
static wt_heapNode_t* add_node_r(wt_heapNode_t* current,wt_heapNode_t* nu);
static wt_heapNode_t* free_node_r(wt_heapNode_t* current,void* ptr,uint32_t *sz);
static uint32_t node_available_r(wt_heapNode_t* current);
static void print_tab(int k);
static void print_r(wt_heapNode_t* current,int k);


static wt_heapNode_t null_node = {
		.left = NULL,
		.right = NULL,
		.pos = NULL,
		.limit = NULL,
		.size = 0,
		.entry = {0}
};

void wt_initRoot(wt_heapRoot_t* root){
	if(!root)
		return;
	root->hnodes = NULL_CACHE;
	root->size = 0;
	root->free_sz = 0;
}

void wt_initNode(wt_heapNode_t* alloc,void* addr,uint32_t sz){
	alloc->base = alloc->pos = (uint8_t*) addr;
	alloc->limit = (void*) ((uint32_t) addr + sz);
	alloc->size = sz;
	alloc->left = alloc->right = NULL_CACHE;
	wtreeRootInit(&alloc->entry,sizeof(struct ext_header));
}


void wt_addNode(wt_heapRoot_t* heap,wt_heapNode_t* cache){
	if(!heap || !node_free)
		return;
	heap->hnodes = add_node_r(heap->hnodes,cache);
	heap->size += cache->size;
	heap->free_sz += cache->size;
}

void* wt_malloc(wt_heapRoot_t* heap,uint32_t sz){
	if(!heap || !sz)
		return NULL;
	void* chunk_ptr = node_malloc(heap->hnodes,sz);
	if((heap->hnodes->left->size > heap->hnodes->size) || (heap->hnodes->right->size > heap->hnodes->size)){
		if(heap->hnodes->left->size > heap->hnodes->right->size){
			heap->hnodes = rotateRight(heap->hnodes);
		}else{
			heap->hnodes = rotateLeft(heap->hnodes);
		}
	}
	heap->free_sz -= sz;
	return chunk_ptr;
}

void wt_free(wt_heapRoot_t* heap,void* ptr){
	if(!heap || !ptr)
		return;
	if(heap->hnodes == NULL_CACHE)
		exit(-1);
	uint32_t sz = 0;
	heap->hnodes = free_node_r(heap->hnodes,ptr,&sz);
	if(sz != 0)
		heap->free_sz += sz;
}

void wt_initCache(wt_cache_t* cache,size_t sz_limit){
	if(!cache)
		return;
	wtreeRootInit(&cache->entry,sizeof(struct ext_header));
	cache->size = 0;
	cache->size_limit = sz_limit;
}

void* wt_cacheMalloc(wt_cache_t* cache,uint32_t sz){
	if(!sz || (cache->size < sz))
		return NULL;

	wtreeNode_t* chunk = wtreeRetrive(&cache->entry,&sz);
	struct heapHeader *chdr,*nhdr,*nnhdr;
	if(chunk == NULL)
		return NULL;

	chdr = (struct heapHeader*) container_of(chunk, struct heapHeader,wtree_node);
	nnhdr = (struct heapHeader*) ((uint32_t) &chdr->wtree_node + sz);
	chdr->size = sz;
	nnhdr->psize = sz;
	cache->size -= sz;
	return &chdr->wtree_node;
}

int wt_cacheFree(wt_cache_t* cache,void* ptr){
	if(!ptr)
		return FALSE;
	if(cache->size_limit >= cache->size)
		return FALSE;

	struct heapHeader* chdr,* nhdr;
	chdr = (struct heapHeader* ) container_of(ptr,struct heapHeader,wtree_node);
	nhdr = (struct heapHeader* ) ((uint32_t) ptr + chdr->size);
	if(chdr->size != nhdr->psize)
		tch_kernel_raise_error(tch_currentThread,tchErrorHeapCorruption,"heap corruption detected");
	wtreeNodeInit(&chdr->wtree_node,(uint32_t) &chdr->wtree_node,chdr->size);
	wtreeInsert(&cache->entry,&chdr->wtree_node);
	cache->size += chdr->size;
	return TRUE;
}

void wt_cacheFlush(wt_heapRoot_t* heap,wt_cache_t* cache){
	if(!heap || !cache)
		return;
	wtreeNode_t* wtn;
	while((wtn = wtreeDeleteRightMost(&cache->entry)) != NULL) {
		wt_free(heap,wtn);
	}
}

uint32_t wt_size(wt_heapRoot_t* heap){
	if(!heap)
		return 0;
	return heap->size;
}

uint32_t wt_available(wt_heapRoot_t* heap){
	if(!heap)
		return 0;
	return heap->free_sz;
}


void wt_print(wt_heapRoot_t* heap){
	if(!heap)
		return;
	print_r(heap->hnodes,0);
}


static wt_heapNode_t* add_node_r(wt_heapNode_t* current,wt_heapNode_t* nu){
	if(current == NULL_CACHE)
		return nu;
	if(current->base < nu->base){
		current->right = add_node_r(current->right,nu);
		if(current->size < current->right->size)
			return rotateLeft(current);
	}else{
		current->left = add_node_r(current->left,nu);
		if(current->size < current->left->size)
			return rotateRight(current);
	}
	return current;
}


/**
 *
 */
static wt_heapNode_t* free_node_r(wt_heapNode_t* current,void* ptr,uint32_t *sz){
	if(current->base > ptr){
		current->left = free_node_r(current->left,ptr,sz);
		if(current->left->size > current->size)
			return rotateRight(current);

	}

	else if((current->base <= ptr) && (current->limit > ptr)) {
		*sz = node_free(current,ptr);
	} else {
		current->right = free_node_r(current->right,ptr,sz);
		if(current->right->size > current->size)
			return rotateLeft(current);
	}
	return current;
}

static uint32_t node_available_r(wt_heapNode_t* current){
	if(current == NULL_CACHE)
		return 0;
	return node_available_r(current->left) + current->size + node_available_r(current->right);
}


static void print_tab(int k){
	while(k--)	printf("\t");
}


static void print_r(wt_heapNode_t* current,int k){
	if(current == NULL_CACHE)
		return;
	print_r(current->left,k + 1);
	print_tab(k);printf("{cache : base %d , size %d @ %d}\n",current->base,current->size,k);
	print_r(current->right,k + 1);
}

static wt_heapNode_t* rotateLeft(wt_heapNode_t* rot_pivot){
	wt_heapNode_t* nparent = rot_pivot->right;
	rot_pivot->right = nparent->left;
	nparent->left = rot_pivot;
	return nparent;
}

static wt_heapNode_t* rotateRight(wt_heapNode_t* rot_pivot){
	wt_heapNode_t* nparent = rot_pivot->left;
	rot_pivot->left = nparent->right;
	nparent->right = rot_pivot;
	return nparent;
}

static void * node_malloc(wt_heapNode_t* alloc,uint32_t sz){
	if(!sz)
		return NULL;
	wtreeNode_t* chunk = wtreeRetrive(&alloc->entry,&sz);
	struct heapHeader *chdr,*nhdr,*nnhdr;
	uint64_t tsz;
	if(chunk == NULL){
		if(alloc->pos + sz + sizeof(struct heapHeader) >= alloc->limit)
			return NULL; // impossible to handle
		chdr = (struct heapHeader*) alloc->pos;
		alloc->pos = (void*) ((uint32_t) &chdr->wtree_node + sz);
		nhdr = (struct heapHeader*) alloc->pos;
		chdr->size = sz;
		nhdr->psize = sz;
	}else{
		chdr = (struct heapHeader *) container_of(chunk,struct heapHeader,wtree_node);  //?
		nnhdr = (struct heapHeader *) ((uint32_t) &chdr->wtree_node + sz);
		chdr->size = sz;
		nnhdr->psize = sz;
	}
	alloc->size -= sz;
	return &chdr->wtree_node;
}

static uint32_t node_free(wt_heapNode_t* alloc,void* ptr){
	if(!ptr)
		return 0;
	struct heapHeader* chdr,*nhdr;
	chdr = (struct heapHeader*) container_of(ptr,struct heapHeader,wtree_node);
	nhdr = (struct heapHeader*) ((uint32_t)ptr + chdr->size);
	if(chdr->size != nhdr->psize)
		Thread->terminate(tch_currentThread,tchErrorNoMemory);
	wtreeNodeInit(&chdr->wtree_node,(uint32_t)&chdr->wtree_node,chdr->size);
	wtreeInsert(&alloc->entry,&chdr->wtree_node);
	alloc->size += chdr->size;
	return chdr->size;
}

static uint32_t node_size(wt_heapNode_t* alloc){
	return wtreeTotalSpan(&alloc->entry);
}


static void node_print(wt_heapNode_t* alloc){
	printf("======================================================================================================\n");
	wtreePrint(&alloc->entry);
	printf("======================================================================================================\n");
}


