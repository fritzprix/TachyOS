/*
 * malloc.h
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 */

#ifndef MALLOC_H_
#define MALLOC_H_

#include <stddef.h>
#include "wtree.h"

#define WT_ERROR	((int) -1)
#define WT_FAIL		((int) 0)
#define WT_OK		((int) 1)


typedef struct wt_heap_node wt_heapNode_t;
typedef struct wt_heaproot wt_heapRoot_t;
typedef struct wt_cache wt_cache_t;

struct wt_heap_node {
	wt_heapNode_t 	*left,*right;
	void*		 	base;
	void* 			pos;
	void* 			limit;
	size_t 			size;
	wtreeRoot_t		entry;
};

struct wt_heaproot {
	wt_heapNode_t*  hnodes;
	size_t		 	size;
	size_t		 	free_sz;
};

struct wt_cache {
	wtreeRoot_t		entry;
	size_t			size;
	size_t			size_limit;
};


extern void wt_initRoot(wt_heapRoot_t* root);
extern void wt_initNode(wt_heapNode_t* aloc,void* addr,uint32_t size);
extern void wt_initCache(wt_cache_t* cache,size_t sz_limit);
extern void wt_addNode(wt_heapRoot_t* heap,wt_heapNode_t* cache);
extern void* wt_malloc(wt_heapRoot_t* heap,uint32_t sz);
extern int wt_free(wt_heapRoot_t* heap,void* ptr);

extern void* wt_cacheMalloc(wt_cache_t* cache,uint32_t sz);
extern int wt_cacheFree(wt_cache_t* cache,void* ptr);
extern void wt_cacheFlush(wt_heapRoot_t* heap,wt_cache_t* cache);

extern uint32_t wt_size(wt_heapRoot_t* heap);
extern uint32_t wt_available(wt_heapRoot_t* heap);

extern void wt_print(wt_heapRoot_t* heap);


#endif /* MALLOC_H_ */
