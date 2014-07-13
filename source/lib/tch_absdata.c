/*
 * tch_absdata.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 18.
 *      Author: innocentevil
 */

#include "tch.h"
#include "tch_absdata.h"


void tch_genericQue_Init(tch_genericList_queue_t* queue){
	queue->entry = NULL;
	queue->tail = NULL;
}


void tch_genericQue_enqueueWithCompare(tch_genericList_queue_t* queue,tch_genericList_node_t* element,tch_list_elementCompare cmp_fnp){
	tch_genericList_node_t* qEl = queue->entry;
	if(qEl == NULL){
		element->prev = NULL;
		element->next = NULL;
		queue->entry = element;
		queue->tail = element;
		return;
	}
	while(qEl->next != NULL){
		if(cmp_fnp(qEl,element)){
			element->prev = qEl->prev;
			element->next = qEl;
			qEl->prev = element;
			if(element->prev == NULL){
				queue->entry = element;
			}else{
				element->prev->next = element;
			}
			return;
		}
		qEl = qEl->next;
	}
	if(cmp_fnp(qEl,element)){
		element->prev = qEl->prev;
		element->next = qEl;
		qEl->prev = element;
		if(element->prev == NULL){
			queue->entry = element;
		}else{
			element->prev->next = element;
		}
		return;
	}
	qEl->next = element;
	element->prev = qEl;
	element->next = NULL;
	queue->tail = element;
}

/**
 *
 */
void tch_genericQue_putAhead(tch_genericList_queue_t* queue,tch_genericList_node_t* element){
	element->next = queue->entry->next;
	queue->entry = element;
	queue->entry->prev = NULL;
}

/**
 *
 */

void tch_genericQue_putTail(tch_genericList_queue_t* queue,tch_genericList_node_t* element){
	element->next = NULL;
	queue->tail->next = element;
	element->prev = queue->tail;
	queue->tail = element;
}

/**
 *
 */
tch_genericList_node_t* tch_genericQue_dequeue(tch_genericList_queue_t* queue){
	tch_genericList_node_t* hnode = queue->entry;
	if(hnode == NULL){
		return NULL;
	}
	queue->entry = hnode->next;
	if(queue->entry != NULL){
		queue->entry->prev = NULL;
	}
	hnode->prev = NULL;
	hnode->next = NULL;
	return hnode;
}

/**
 *
 */

int tch_genericQue_remove(tch_genericList_queue_t* queue,tch_genericList_node_t* element){
	tch_genericList_node_t* ql_node = queue->entry;
	if(!ql_node){
		return FALSE;
	}
	while(ql_node){
		if(element == ql_node){
			if(ql_node->prev){
				ql_node->prev->next = ql_node->next;
				if(ql_node->next)
					ql_node->next->prev = ql_node->prev;
			}else{
				queue->entry = ql_node->next;
				queue->entry->prev = NULL;
			}
			return TRUE;
		}
		ql_node = ql_node->next;
	}
	return FALSE;
}



void tch_genericMemPool_init(tch_generic_ringBuffer* mp,void* bp,uint8_t align,uint32_t len){
	mp->_bp = bp;
	mp->balign = align;
	mp->blastIdx = 0;
	mp->blenght = len;
	uint32_t bytelen = mp->balign * mp->blenght;
	uint8_t* tbp = mp->_bp;
	while(bytelen--){
		*tbp++ = 0;
	}
	mp->blastIdx = 0;
}

void* tch_genericMemPool_allcate(tch_generic_ringBuffer* mp){
	uint32_t searchMaxTry = (mp->blenght >> 2 | 0x01);
	uint32_t bytelen = mp->balign * mp->blenght;
	uint32_t idx = mp->blastIdx;
	while(searchMaxTry--){
		if(!(idx < bytelen)){
			idx = 0;
		}
		if(mp->bstub.isAvailable((uint8_t*)mp->_bp + idx)){
			mp->blastIdx = idx;
			return (uint8_t*) mp->_bp + idx;
		}
		idx += mp->balign;
	}
	return NULL;
}

void tch_genericMemPool_free(tch_generic_ringBuffer* mp,void* ins){
	mp->bstub.onDealloc(ins);
}

