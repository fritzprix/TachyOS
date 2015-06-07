/*
 * tch_list.c
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */

#include "cdsl_dlist.h"



void cdsl_dlistInit(cdsl_dlistNode_t* lentry){
	lentry->next = NULL;
	lentry->prev = NULL;
}

void cdsl_dlistEnqueuePriority(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item,cdsl_dlistPriorityRule rule){
	cdsl_dlistNode_t* cnode = lentry;
	if(!item)
		return;
	while(cnode->next != NULL){
		cnode = cnode->next;
		if(rule(cnode,item) == item){
			((cdsl_dlistNode_t*)cnode->prev)->next = item;
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

cdsl_dlistNode_t* cdsl_dlistDequeue(cdsl_dlistNode_t* lentry){
	cdsl_dlistNode_t* cnode = lentry->next;
	if(cdsl_dlistIsEmpty(lentry))
		return NULL;
	lentry->next = cnode->next;
	if(cnode->next)
		((cdsl_dlistNode_t*)cnode->next)->prev = lentry;
	else
		lentry->prev = NULL;
	return cnode;
}

void cdsl_dlistPutHead(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item){
	if(!lentry)
			return;
	item->next = lentry->next;
	if(lentry->next)
		((cdsl_dlistNode_t*)lentry->next)->prev = item;
	else
		lentry->prev = item;
	item->prev = lentry;
	lentry->next = item;
}

void cdsl_dlistInsertAfter(cdsl_dlistNode_t* ahead,cdsl_dlistNode_t* item){
	if(!ahead || !item)
		return;

	item->next = ahead->next;
	item->prev = ahead;
	ahead->next = item;
	if(item->next)
		item->next->prev = item;
}



void cdsl_dlistPutTail(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item){
	if(!lentry || !item)
		return;
	if(lentry->next){
		((cdsl_dlistNode_t*)lentry->prev)->next = item;
		item->prev = lentry->prev;
	} else{
		lentry->next = item;
		lentry->prev = item;
		item->prev = lentry;
	}
	item->next = NULL;
	lentry->prev = item;
}


cdsl_dlistNode_t* cdsl_dlistGetHead(cdsl_dlistNode_t* lentry){
	if(!lentry)
		return NULL;
	return lentry->next;
}

cdsl_dlistNode_t* cdsl_dlistGetTail(cdsl_dlistNode_t* lentry){
	cdsl_dlistNode_t* last = NULL;
	if(!lentry)
		return NULL;
	if(!lentry->prev)
		return NULL;
	last = lentry->prev;
	lentry->prev = last->prev;
	((cdsl_dlistNode_t*)lentry->prev)->next = NULL;
	return last;
}

BOOL cdsl_dlistRemove(cdsl_dlistNode_t* item){
	if(!item)
		return FALSE;
	cdsl_dlistNode_t* prev = item->prev;
	if(prev)
		prev->next = item->next;
	if(item->next)
		item->next->prev = prev;
	return TRUE;
}

int cdsl_dlistSize(cdsl_dlistNode_t* lentry){
	int cnt = 0;
	cdsl_dlistNode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		cnt++;
	}
	return cnt;
}

BOOL cdsl_dlistContain(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item){
	cdsl_dlistNode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
		if(cnode == item){
			return TRUE;
		}
	}
	return FALSE;
}

void cdsl_dlistPrint(cdsl_dlistNode_t* lentry,cdsl_generic_printer_t prt){
	cdsl_dlistNode_t* cnode = lentry;
	if(!prt)
		return;
	while(cnode->next != NULL){
		cnode = cnode->next;
		prt(cnode);
	}
}
