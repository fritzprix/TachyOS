#include "../../../include/kernel/mm/owtree.h"

/*
 * wtree.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 *
 *      ==================================== wwtree (width-weighted tree) =========================================
 *      dynamic memory allocation has been widely supported in various systems while only a few light weight system
 *      don't support it to achieve strict real time performance or cost efficiency.
 *      in rich featured architecture, like ARM Cortex-A Series which supports MMU hardware. fragmentation due to
 *      dynamic memory allocation is relatively low cost than MMU-less hardware, because MMU provides address mapping
 *      efficiently. application doesn't actually notice the seam between discrete memory chunck in physical memory
 *      address space.
 *
 *      in MMU-less hardware, software should take care of dynamic memory request from application without compromise of
 *      performance while managing fragmentation issue.
 *      I devised wwtree for this application in mind. wwtree lookup avaiable chunk at O(1) performance while efficiently
 *      merge fragmented chunk with best-effort strategy at free operation which is performance of O(logn).
 *      this is very suitable to MMU-less hardware based time critical system which has been target of many RTOS.
 *
 *
 *
 */

#include "tch_ktypes.h"

static owtreeNode_t null_node = {
	.left = NULL,
	.right = NULL,
	.base = 0,
	.span = 0
};

#define NULL_NODE   &null_node


static owtreeNode_t* insert_r(owtreeNode_t* current,owtreeNode_t* item,uint32_t gap);
static owtreeNode_t* rotateLeft(owtreeNode_t* rot_pivot);
static owtreeNode_t* rotateRight(owtreeNode_t* rot_pivot);
static owtreeNode_t* mergeSubtree(owtreeNode_t* root);
static size_t span_r(owtreeNode_t* node);
static void print_r(owtreeNode_t* node,int depth);
static void print_tab(int k);

void owtreeRootInit(owtreeRoot_t* root,uint32_t ext_gap){
	root->entry = NULL_NODE;
	root->ext_gap = ext_gap;
}

void owtreeNodeInit(owtreeNode_t* node,uint32_t base,uint32_t span){
	node->left = node->right = NULL_NODE;
	node->base = base;
	node->span = span;
}

size_t owtreeTotalSpan(owtreeRoot_t* root){
	return span_r(root->entry);
}

void owtreeInsert(owtreeRoot_t* root,owtreeNode_t* item){
	if(!root)
		return;
	root->entry = insert_r(root->entry,item,root->ext_gap);
}

owtreeNode_t* owtreeRetrive(owtreeRoot_t* root,uint32_t* span){
	if(!root || !root->entry)
		return NULL;
	uint32_t nspan = *span + root->ext_gap;
	if(root->entry->span < *span)
		return NULL;

	owtreeNode_t* retrived = NULL;
	if(root->entry->span >= (nspan + sizeof(owtreeNode_t))){			//
		// build up new chunk
		retrived = root->entry;
		root->entry = (owtreeNode_t*)(((uint8_t*) root->entry) + nspan);
		nspan = retrived->span - nspan;
		retrived->span = *span;
		owtreeNodeInit(root->entry,(uint32_t)root->entry,nspan);
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

owtreeNode_t* owtreeDeleteRightMost(owtreeRoot_t*root){
	if(!root)
		return NULL;
	if(root->entry == NULL_NODE)
		return NULL;
	owtreeNode_t* rm;
	owtreeNode_t ** current;
	current = &root->entry;
	while((*current)->right != NULL_NODE){
		current = &(*current)->right;
	}
	rm = *current;
	if(rm->left == NULL_NODE){
		*current = NULL;
	}else {
		*current = rm->left;
	}
	return rm;
}



void owtreePrint(owtreeRoot_t* root){
	print_r(root->entry,0);
}


static owtreeNode_t* insert_r(owtreeNode_t* current,owtreeNode_t* item,uint32_t gap){
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

static void print_r(owtreeNode_t* node,int depth){
	if(node == NULL_NODE)
		return;
	print_r(node->right,depth + 1);
//	print_tab(depth);printf("{base : %d , span : %d} @ depth %d\n",node->base,node->span,depth);
	print_r(node->left,depth + 1);

}


static void print_tab(int k){
	//while(k--)printf("\t");
}

static owtreeNode_t* rotateLeft(owtreeNode_t* rot_pivot){
	owtreeNode_t* nparent = rot_pivot->right;
	rot_pivot->right = nparent->left;
	nparent->left = rot_pivot;
	return nparent;
}

static owtreeNode_t* rotateRight(owtreeNode_t* rot_pivot){
	owtreeNode_t* nparent = rot_pivot->left;
	rot_pivot->left = nparent->right;
	nparent->right = rot_pivot;
	return nparent;
}

static owtreeNode_t* mergeSubtree(owtreeNode_t* root){
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


static size_t span_r(owtreeNode_t* node){
	if(node == NULL_NODE)
		return 0;
	return node->span + span_r(node->right) + span_r(node->left);
}


