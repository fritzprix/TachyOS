/*
 * generic_data_types.h
 *
 *  Created on: 2014. 4. 13.
 *      Author: innocentevil
 */

#ifndef GENERIC_DATA_TYPES_H_
#define GENERIC_DATA_TYPES_H_
#include "../core/port/cortex_v7m_port.h"


typedef struct _tch_list_node_t tch_genericList_node_t;
typedef struct _tch_list_queue_t tch_genericList_queue_t;                                           ///   **
typedef struct _tch_generic_ringBuffer_t tch_generic_ringBuffer;                                      ///   *** Generic typed Memory Pool base on Ring buffer Structure
typedef struct _tch_generic_ringBuffer_stub_t tch_generic_ringBuffer_stub;



typedef int (*tch_list_elementCompare)(void* li0,void* li1);                      /// should return ge0 > ge1

#define GENERIC_LIST_NODE_INIT                 {NULL,NULL}
#define GENERIC_LIST_QUEUE_INIT                {NULL,NULL}

#define LIST_COMPARE_FN(fname) int fname(void* li0,void* li1)

struct _tch_list_node_t {
	tch_genericList_node_t* next;
	tch_genericList_node_t* prev;
};

struct _tch_list_queue_t {
	tch_genericList_node_t* entry;
	tch_genericList_node_t* tail;
};


struct _tch_generic_ringBuffer_stub_t{
	BOOL  (*isAvailable)(void*);
	void*    (*onDealloc)(void*);
};



struct _tch_generic_ringBuffer_t {
	void*                       _bp;
	uint8_t                      balign;
	uint32_t                     blenght;
	uint32_t                     blastIdx;
	tch_generic_ringBuffer_stub   bstub;
};


void tch_genericQue_Init(tch_genericList_queue_t* queue);
void tch_genericQue_enqueueWithCompare(tch_genericList_queue_t* queue,tch_genericList_node_t* element,tch_list_elementCompare cmp_fnp);
void tch_genericQue_putAhead(tch_genericList_queue_t* queue,tch_genericList_node_t* element);
void tch_genericQue_putTail(tch_genericList_queue_t* queue,tch_genericList_node_t* element);
tch_genericList_node_t* tch_genericQue_dequeue(tch_genericList_queue_t* queue);
int tch_genericQue_remove(tch_genericList_queue_t* queue,tch_genericList_node_t* element);


void tch_genericMemPool_init(tch_generic_ringBuffer* mp,void* bp,uint8_t align,uint32_t len);
void* tch_genericMemPool_allcate(tch_generic_ringBuffer* mp);
void tch_genericMemPool_free(tch_generic_ringBuffer* mp,void* ins);




#endif /* GENERIC_DATA_TYPES_H_ */
