/*
 * cdsl_slist.c
 *
 *  Created on: 2015. 5. 31.
 *      Author: innocentevil
 */


#include "cdsl.h"
#include "cdsl_slist.h"



void cdsl_slistInit(cdsl_slistNode_t* lentry){
	lentry->next = NULL;
}


void cdsl_slistEnqueuePriority(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item,cdsl_slistPriorityRule rule){
	cdsl_slistNode_t** cnode = &lentry;
	while((*cnode)->next != NULL){
		if(rule((*cnode)->next,item) == item){
			item->next = (*cnode)->next;
			(*cnode)->next = item;
			return;
		}
		cnode = &(*cnode)->next;
	}
	(*cnode)->next = item;
}

void cdsl_slistInsertAfter(cdsl_slistNode_t* ahead,cdsl_slistNode_t* item){
	if(!ahead || !item)
		return;
	item->next = ahead->next;
	ahead->next = item;
}

cdsl_slistNode_t* cdsl_slistDequeue(cdsl_slistNode_t* lentry){
	if(!lentry)
		return NULL;
	cdsl_slistNode_t* head = lentry->next;
	if(head)
		lentry->next = head->next;
	return head;
}


void cdsl_slistPutHead(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item){
	if(!lentry || !item)
		return;
	if(lentry->next)
		item->next = lentry->next->next;
	else
		item->next = NULL;
	lentry->next = item;
}

void cdsl_slistPutTail(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item){
	if(!lentry || !item)
		return;
	item->next = NULL;
	cdsl_slistNode_t* cnode = lentry;
	while(cnode->next != NULL){
		cnode = cnode->next;
	}
	cnode->next = item;
}

cdsl_slistNode_t* cdsl_slistGetHead(cdsl_slistNode_t* lentry){
	return cdsl_slistDequeue(lentry);
}

cdsl_slistNode_t* cdsl_slistGetTail(cdsl_slistNode_t* lentry){
	if(!lentry)
		return NULL;
	cdsl_slistNode_t* tail = NULL;
	cdsl_slistNode_t** cnode = &lentry;
	while((*cnode)->next != NULL){
		cnode = &(*cnode)->next;
	}
	tail = (*cnode);
	*cnode = NULL;
	return tail;
}

BOOL cdsl_slistRemove(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item){
	if(!lentry || !item)
		return FALSE;
	cdsl_slistNode_t* cnode = lentry;
	while(cnode->next != NULL){
		if(cnode->next == item){
			cnode->next = item->next;
			return TRUE;
		}
		cnode = cnode->next;
	}
	return FALSE;
}

int cdsl_slistSize(cdsl_slistNode_t* lentry){
	if(!lentry)
		return FALSE;
	int cnt = 0;
	cdsl_slistNode_t* cnode = lentry;
	while(cnode->next){
		cnode = cnode->next;
		cnt++;
	}
	return cnt;
}

BOOL cdsl_slistContain(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item){
	if(!lentry)
		return FALSE;
	cdsl_slistNode_t* cnode = lentry;
	while(cnode->next){
		if(cnode->next == item)
			return TRUE;
		cnode = cnode->next;
	}
	return FALSE;
}

void cdsl_slistPrint(cdsl_slistNode_t* lentry,cdsl_generic_printer_t prt){
	if(!lentry || !prt)
		return;
	cdsl_slistNode_t* cnode = lentry;
	while(cnode->next){
		cnode = cnode->next;
		prt(cnode->next);
	}
}
