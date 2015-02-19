/*
 * tch_ltree.c
 *
 *  Created on: 2014. 9. 26.
 *      Author: innocentevil
 */

#include <stddef.h>

#include "tch_ltree.h"

#include "tch_btree.h"
#include "tch_list.h"

void tch_ltreeInit(tch_ltree_node* root,int key){
	tch_listInit(&root->ln);
	tch_btreeInit(&root->trn,key);
}

tch_ltree_node* tch_ltreeInsert(tch_ltree_node* root, tch_ltree_node* item){
	tch_listPush(&root->ln,&item->ln);
	return (tch_ltree_node*)tch_btree_insert(&root->trn,&item->trn);
}

tch_ltree_node* tch_ltreeLookup(tch_ltree_node* root, int key){
	return (tch_ltree_node*) tch_btree_lookup(&root->trn,key);
}

tch_ltree_node* tch_ltreeRemove(tch_ltree_node** rtp,int key){
	tch_ltree_node* root = *rtp;
	tch_ltree_node* ntr =  (tch_ltree_node*) tch_btree_delete((tch_ltree_node**) rtp,key);
	if(!ntr)
		return NULL;
	tch_listRemove(&root->ln,&ntr->ln);
	return ntr;
}

tch_ltree_node* tch_ltreeRemoveHead(tch_ltree_node** rtp){
	tch_ltree_node* root = *rtp;
	root = tch_listDequeue(&root->ln);
	if(!root)
		return NULL;
	root = (tch_ltree_node*)((tch_btree_node*)root - 1);
	return (tch_ltree_node*) tch_btree_delete((tch_ltree_node**) rtp,root->trn.key);
}

tch_ltree_node* tch_ltreeRemoveTail(tch_ltree_node** rtp){
	tch_ltree_node* root = *rtp;
	root = tch_listPop(&root->ln);
	if(!root)
		return NULL;
	root = (tch_ltree_node*)((tch_btree_node*)root - 1);
	return (tch_ltree_node*) tch_btree_delete((tch_ltree_node**) rtp,root->trn.key);
}



void tch_ltreePrint(tch_ltree_node* root,void (*prt)(void*)){
	tch_listPrint(&root->ln,prt);
}
