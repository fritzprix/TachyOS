/*
 * heap.c
 *
 *  Created on: 2015. 5. 3.
 *      Author: innocentevil
 */


#include "cdsl_heap.h"
#include <stddef.h>


/**
 * recursive interanl function
 */
static cdsl_heapNode_t* cdsl_heapInsertFromBottom(cdsl_heapNode_t* current,cdsl_heapNode_t* item,heapEvaluate eval);
static cdsl_heapNode_t* cdsl_heapMoveNodeDown(cdsl_heapNode_t* current,heapEvaluate eval);
static cdsl_heapNode_t* cdsl_heapGetLeafNode(cdsl_heapNode_t* current);


int cdsl_heapEnqueue(cdsl_heapNode_t** heap,cdsl_heapNode_t* item,heapEvaluate eval){
	item->left = item->right = NULL;
	if(!heap)
		return (1 < 0);
	*heap = cdsl_heapInsertFromBottom(*heap,item,eval);
	return (1 > 0);
}

cdsl_heapNode_t* cdsl_heapDeqeue(cdsl_heapNode_t** heap,heapEvaluate eval){
	if(!heap)
		return NULL;
	cdsl_heapNode_t* current = *heap;
	*heap = cdsl_heapGetLeafNode(*heap);
	if(current == *heap){
		*heap = NULL;
		return current;
	}
	(*heap)->left = current->left;
	(*heap)->right = current->right;

	*heap = cdsl_heapMoveNodeDown(*heap,eval);
	return current;
}


void cdsl_heapPrint(cdsl_heapNode_t** root,heapPrint prt){
	if(!root || !*root)
		return;
	prt(*root);
	cdsl_heapPrint(&(*root)->right,prt);
	cdsl_heapPrint(&(*root)->left,prt);
}


static cdsl_heapNode_t* cdsl_heapInsertFromBottom(cdsl_heapNode_t* current,cdsl_heapNode_t* item,heapEvaluate eval){
	cdsl_heapNode_t* child,*left,*right = NULL;
	if(!current){
//		DBG_PRT("Current is Null Node\n");
		return item;
	}
	current->flipper = !current->flipper;
	if(current->flipper){
//		DBG_PRT("Go Right\n");
		child = cdsl_heapInsertFromBottom(current->right,item,eval);
		if(current->right != child){
			if(current != eval(current,child)){
				left = child->left;
				right = child->right;
				child->left = current->left;
				child->right = current;
				current->left = left;
				current->right = right;
				child->right = current;
				return child;
			}else{
				current->right = child;
				return current;
			}
		}
		return current;
	} else {
//		DBG_PRT("Go Left\n");
		child = cdsl_heapInsertFromBottom(current->left,item,eval);
		if(current->left != child){
			if(current != eval(current,child)){
				left = child->left;
				right = child->right;
				child->left = current;
				child->right = current->right;
				current->left = left;
				current->right = right;
				child->left = current;
				return child;
			}else{
				current->left = child;
				return current;
			}
		}
		return current;
	}
}



static cdsl_heapNode_t* cdsl_heapGetLeafNode(cdsl_heapNode_t* current){
	cdsl_heapNode_t* leaf = NULL;
	if(!current) return leaf;
	if(!current->right && !current->left){
		return current;
	}
	if (current->right) {
		leaf = cdsl_heapGetLeafNode(current->right);
		if (leaf) {
			if (leaf == current->right)
				current->right = NULL;
			return leaf;
		}
	}
	if (current->left) {
		leaf = cdsl_heapGetLeafNode(current->left);
		if (leaf) {
			if (leaf == current->left)
				current->left = NULL;
			return leaf;
		}
	}
	return current;
}

static cdsl_heapNode_t* cdsl_heapMoveNodeDown(cdsl_heapNode_t* current,heapEvaluate eval){
	cdsl_heapNode_t* largest = current,*left = NULL,*right = NULL;
	if(!largest)
		return NULL;
	largest = eval(largest,current->right);
	largest = eval(largest,current->left);

	if(largest == current->left){
		left = largest->left;
		right = largest->right;
		largest->right = current->right;
		current->right = right;
		current->left = left;
		largest->left = cdsl_heapMoveNodeDown(current,eval);
	} else if(largest == current->right){
		left = largest->left;
		right = largest->right;
		largest->left =current->left;
		current->right = right;
		current->left = left;
		largest->right = cdsl_heapMoveNodeDown(current,eval);
	}
	return largest;
}


