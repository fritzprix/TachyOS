/*
 * tch_list.c
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */


#include <stddef.h>
#include <stdio.h>

#include "util/tch_list.h"



void tch_listInit(tch_lnode_t* lentry){
	lentry->next = NULL;
	lentry->prev = NULL;
}

void tch_listEnqueuePriority(tch_lnode_t* lentry,tch_lnode_t* item,int (*cmp)(void* prior,void* post)){
	tch_lnode_t* cnode = lentry;
	if(!item)
		return;
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

tch_lnode_t* tch_listDequeue(tch_lnode_t* lentry){
	tch_lnode_t* cnode = lentry->next;
	if(tch_listIsEmpty(lentry))
		return NULL;
	lentry->next = cnode->next;
	if(cnode->next)
		((tch_lnode_t*)cnode->next)->prev = lentry;
	else
		lentry->prev = NULL;
	return cnode;
}

void tch_listPutHead(tch_lnode_t* lentry,tch_lnode_t* item){
	if(!lentry)
			return;
	item->next = lentry->next;
	if(lentry->next)
		((tch_lnode_t*)lentry->next)->prev = item;
	else
		lentry->prev = item;
	item->prev = lentry;
	lentry->next = item;
}


void tch_listPutTail(tch_lnode_t* lentry,tch_lnode_t* item){
	if(!lentry || !item)
		return;
	if(lentry->next){
		((tch_lnode_t*)lentry->prev)->next = item;
		item->prev = lentry->prev;
	} else{
		lentry->next = item;
		lentry->prev = item;
		item->prev = lentry;
	}
	item->next = NULL;
	lentry->prev = item;
}


tch_lnode_t* tch_listGetHead(tch_lnode_t* lentry){
	if(!lentry)
		return NULL;
	return lentry->next;
}

tch_lnode_t* tch_listGetTail(tch_lnode_t* lentry){
	tch_lnode_t* last = NULL;
	if(!lentry)
		return NULL;
	if(!lentry->prev)
		return NULL;
	last = lentry->prev;
	lentry->prev = last->prev;
	((tch_lnode_t*)lentry->prev)->next = NULL;
	return last;
}

int tch_listRemove(tch_lnode_t* lentry,tch_lnode_t* item){
	if(tch_listIsEmpty(lentry))
		return (1 < 0);
	if(!item)
		return (1 < 0);
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		if(cnode == item){
			if(cnode->next){
				cnode->next->prev = cnode->prev;
			}
			if(cnode->prev){
				cnode->prev->next = cnode->next;
				if(!lentry->next){
					lentry->prev = NULL;
				}
			}
			return (1 > 0);
		}
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
	tch_lnode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		printitem(cnode);
	}
}
