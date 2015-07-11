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


typedef struct wt_alloc wt_alloc_t;

struct wt_alloc {
	wt_alloc_t *left,*right;
	void*	 	base;
	void* 		pos;
	void* 		limit;
	size_t 		size;
	wtreeRoot_t	entry;
};

typedef struct wt_heaproot {
	wt_alloc_t*  cache;
	size_t		 size;
	size_t		 free_sz;
}wt_heaproot_t;


extern void wtreeHeap_initCacheRoot(wt_heaproot_t* root);
extern void wtreeHeap_initCacheNode(wt_alloc_t* aloc,void* addr,size_t size);
extern void wtreeHeap_addCache(wt_heaproot_t* heap,wt_alloc_t* cache);
extern void* wtreeHeap_malloc(wt_heaproot_t* heap,size_t sz);
extern void wtreeHeap_free(wt_heaproot_t* heap,void* ptr);
extern size_t wtreeHeap_size(wt_heaproot_t* heap);
extern size_t wtreeHeap_available(wt_heaproot_t* heap);
extern void wtreeHeap_print(wt_heaproot_t* heap);


#endif /* MALLOC_H_ */
