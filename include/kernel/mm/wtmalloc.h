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

typedef struct wt_alloc {
	void*	 	heap_base;
	void* 		heap_pos;
	void* 		heap_limit;
	size_t 		heap_size;
	wtreeRoot_t	cache_root;
}wt_alloc_t;

extern void wtreeHeap_init(wt_alloc_t* aloc,void* addr,size_t size);
extern void * wtreeHeap_malloc(wt_alloc_t* aloc,size_t sz);
extern void wtreeHeap_free(wt_alloc_t* aloc,void* ptr);
extern size_t wtreeHeap_size(wt_alloc_t* aloc);
extern void wtreeHeap_print(wt_alloc_t* aloc);


#endif /* MALLOC_H_ */
