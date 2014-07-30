/*
 * tch_list.c
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */


#include <stddef.h>
#include <stdio.h>

#include "tch_list.h"



void tch_listInit(tch_lnode_t* lentry){
	lentry->next = NULL;
	lentry->prev = NULL;
}

void tch_listEnqueuePriority(tch_lnode_t* lentry,tch_lnode_t* item,int (*cmp)(void* prior,void* post)){
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		if(!cmp(cnode,item)){
			((tch_lnode_t*)cnode->prev)->next = item;
			item->prev = cnode->prev;
			item->next = cnode;
			cnode->prev = item;
			return;
		}
	}
	cnode->next = item;
	item->prev = cnode;
	item->next = NULL;
	if(cnode != lentry)
		lentry->prev = item;
}

void* tch_listDequeue(tch_lnode_t* lentry){
	tch_lnode_t* cnode = lentry->next;
	lentry->next = cnode->next;
	if(cnode->next)
		((tch_lnode_t*)cnode->next)->prev = lentry;
	else
		lentry->prev = NULL;
	return (void*) cnode;
}

void tch_listPutFirst(tch_lnode_t* lentry,tch_lnode_t* item){
	item->next = lentry->next;
	if(lentry->next)
		((tch_lnode_t*)lentry->next)->prev = item;
	item->prev = lentry;
	lentry->next = item;
}

void tch_listPutLast(tch_lnode_t* lentry,tch_lnode_t* item){
	if(lentry->prev){
		((tch_lnode_t*)lentry->prev)->next = item;
		item->prev = lentry->prev;
	}
	else{
		lentry->next = item;
		item->prev = lentry;
	}
	item->next = NULL;
	lentry->prev = item;
}

int tch_listRemove(tch_lnode_t* lentry,tch_lnode_t* item){
	if(tch_listIsEmpty(lentry))
		return (1 < 0);
	tch_lnode_t* cnode = lentry->next;
	while(cnode->next != NULL){
		if(cnode == item){
			((tch_lnode_t*)cnode->prev)->next = cnode->next;
			((tch_lnode_t*)cnode->next)->prev = cnode->prev;
			return (1 > 0);
		}
		cnode = cnode->next;
	}
	return (1 < 0);
}

int tch_listSize(tch_lnode_t* lentry){
	int cnt = 0;
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		cnt++;
	}
	return cnt;
}

int tch_listContain(tch_lnode_t* lentry,tch_lnode_t* item){
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		if(cnode == item){
			return (1 > 0);
		}
	}
	return (1 < 0);
}

void tch_listPrint(tch_lnode_t* lentry,void (*printitem)(void* item)){
	printf("\n----------List Print---------------\n");
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		printitem(cnode);
	}
}