/*
 * wtree.c
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 */

#include "tch_ktypes.h"
#include "wtree.h"

static wtreeNode_t null_node = {
	.left = NULL,
	.right = NULL,
	.base = 0,
	.span = 0
};

#define NULL_NODE   &null_node


static wtreeNode_t* insert_r(wtreeNode_t* current,wtreeNode_t* item,uint32_t gap);
static wtreeNode_t* rotateLeft(wtreeNode_t* rot_pivot);
static wtreeNode_t* rotateRight(wtreeNode_t* rot_pivot);
static wtreeNode_t* mergeSubtree(wtreeNode_t* root);
static size_t span_r(wtreeNode_t* node);
static void print_r(wtreeNode_t* node,int depth);
static void print_tab(int k);

void wtreeRootInit(wtreeRoot_t* root,uint32_t ext_gap){
	root->entry = NULL_NODE;
	root->ext_gap = ext_gap;
}

void wtreeNodeInit(wtreeNode_t* node,uint32_t base,uint32_t span){
	node->left = node->right = NULL_NODE;
	node->base = base;
	node->span = span;
}

size_t wtreeTotalSpan(wtreeRoot_t* root){
	return span_r(root->entry);
}


void wtreeInsert(wtreeRoot_t* root,wtreeNode_t* item){
	if(!root)
		return;
	root->entry = insert_r(root->entry,item,root->ext_gap);
}

wtreeNode_t* wtreeRetrive(wtreeRoot_t* root,uint32_t* span){
	if(!root || !root->entry)
		return NULL;
	uint32_t nspan = *span + root->ext_gap;
	if(root->entry->span < *span)
		return NULL;

	wtreeNode_t* retrived = NULL;
	if(root->entry->span >= (nspan + sizeof(wtreeNode_t))){			//
		// build up new chunk
		retrived = root->entry;
		root->entry = (wtreeNode_t*)(((uint8_t*) root->entry) + nspan);
		nspan = retrived->span - nspan;
		retrived->span = *span;
		wtreeNodeInit(root->entry,(uint32_t)root->entry,nspan);
		root->entry->left = retrived->left;
		root->entry->right = retrived->right;
		if((root->entry->right->span > root->entry->span) || (root->entry->left->span > root->entry->span)){
			if(root->entry->right->span > root->entry->left->span){
				root->entry = rotateLeft(root->entry);
			}else {
				root->entry = rotateRight(root->entry);
			}
		}
	}else if(root->entry->span >= nspan){
		retrived = root->entry;
		*span = retrived->span;
		root->entry = mergeSubtree(root->entry);
	}else{
		return NULL;
	}
	return retrived;
}


void wtreePrint(wtreeRoot_t* root){
	print_r(root->entry,0);
}


static wtreeNode_t* insert_r(wtreeNode_t* current,wtreeNode_t* item,uint32_t gap){
	if(current == NULL_NODE)
		return item;
	if(current->base < item->base){
		current->right = insert_r(current->right,item,gap);
		if(current->base + current->span + gap == current->right->base){
			// if two condecutive chunk is mergeable, then merge them
			current->span += current->right->span + gap;
			current->right = current->right->right;
		}else if((current->right->left == item) && (current->base + current->span + gap == current->right->left->base)){
			current->span += current->right->left->span + gap;
			current->right->left = mergeSubtree(current->right->left);
		}
			// if not mergeable, compare two consecutive size
		if(current->span < current->right->span){
			return rotateLeft(current);
		}
	}else{
		current->left = insert_r(current->left,item,gap);
		if(current->left->base + current->left->span + gap == current->base){
			// if two condecutive chunk is mergeable, then merge them
			current = rotateRight(current);
			current->span += current->right->span + gap;
			current->right = current->right->right;
		}else if((current->left->right == item) && (current->left->right->base + current->left->right->span + gap == current->base)){
			current->left = rotateLeft(current->left);
			current = rotateRight(current);
			current->span += current->right->span + gap;
			current->right = mergeSubtree(current->right);
		}
		if(current->span < current->left->span){
			return rotateRight(current);
		}
	}
	return current;
}

static void print_r(wtreeNode_t* node,int depth){
	if(node == NULL_NODE)
		return;
	print_r(node->right,depth + 1);
	print_tab(depth);printf("{base : %d , span : %d} @ depth %d\n",node->base,node->span,depth);
	print_r(node->left,depth + 1);

}

static void print_tab(int k){
	while(k--)printf("\t");
}

static wtreeNode_t* rotateLeft(wtreeNode_t* rot_pivot){
	wtreeNode_t* nparent = rot_pivot->right;
	rot_pivot->right = nparent->left;
	nparent->left = rot_pivot;
	return nparent;
}

static wtreeNode_t* rotateRight(wtreeNode_t* rot_pivot){
	wtreeNode_t* nparent = rot_pivot->left;
	rot_pivot->left = nparent->right;
	nparent->right = rot_pivot;
	return nparent;
}

static wtreeNode_t* mergeSubtree(wtreeNode_t* root){
	if(root->left != NULL_NODE){
		while(root->left->right != NULL_NODE){
			root->left = rotateLeft(root->left);
		}
		root->left->right = root->right;
		return root->left;
	}
	if(root->right != NULL_NODE){
		while(root->right->left != NULL_NODE){
			root->right = rotateRight(root->right);
		}
		root->right->left = root->left;
		return root->right;
	}else{
		return NULL_NODE;
	}
}


static size_t span_r(wtreeNode_t* node){
	if(node == NULL_NODE)
		return 0;
	return node->span + span_r(node->right) + span_r(node->left);
}


