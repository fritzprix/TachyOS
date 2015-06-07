/*
 * tch_btree.c
 *
 *  Created on: 2014. 9. 12.
 *      Author: innocentevil
 */


#include "cdsl_bstree.h"
#include "cdsl.h"
#include <stdio.h>
#include <stddef.h>


static void print_r(bs_treeNode_t* current,cdsl_generic_printer_t prt,int depth);
static void print_tab(int cnt);

void cdsl_bstreeNodeInit(bs_treeNode_t* node,int key){
	node->key = key;
	node->left = NULL;
	node->right = NULL;
}

static bs_treeNode_t* tch_btree_get_rightmost(bs_treeNode_t* node){
	if(!node) return NULL;
	if(!node->right) return node;
	bs_treeNode_t* rightmost = tch_btree_get_rightmost(node->right);
	node->right = rightmost->left;
	rightmost->left = node;
	return rightmost;
}

static bs_treeNode_t* tch_btree_get_leftmost(bs_treeNode_t* node){
	if(!node) return NULL;
	if(!node->left) return node;
	bs_treeNode_t* leftmost = tch_btree_get_leftmost(node->left);
	node->left = leftmost->right;
	leftmost->right = node;
	return leftmost;
}


bs_treeNode_t* cdsl_bstreeInsert(bs_treeNode_t** root,bs_treeNode_t* item){
	if(!root || !item)
		return NULL;
	if(!*root){
		*root = item;
		return item;
	}
	item->left = NULL;
	item->right = NULL;
	bs_treeNode_t* current = *root;
	while(current){
		if(current->key < item->key){
			if(!current->right){
				current->right = item;
				return item;
			}
			current = current->right;
		}else{
			if(!current->left){
				current->left = item;
				return item;
			}
			current = current->left;
		}
	}
	return NULL;
}

bs_treeNode_t* cdsl_bstreeLookup(bs_treeNode_t* root,int key){
	if(!root)
		return NULL;
	while(root && (root->key != key)){
		if(root->key < key)
			root = root->right;
		else
			root = root->left;
	}
	return root;
}

bs_treeNode_t* cdsl_bstreeDelete(bs_treeNode_t** root,int key){

	bs_treeNode_t* todelete = NULL;
	if(!root || !*root)
		return NULL;
	bs_treeNode_t** current = root;
	while((*current) && ((*current)->key != key)){
		if((*current)->key < key){
			current = &(*current)->right;
		}else{
			current = &(*current)->left;
		}
	}
	todelete = *current;
	if (todelete->left) {
		*current = tch_btree_get_rightmost(todelete->left);
		if (*current) {
			(*current)->right = todelete->right;
		}
	}else if(todelete->right){
		*current = tch_btree_get_leftmost(todelete->right);
		(*current)->left = todelete->left;
	}else{
		*current = NULL;
	}
	return todelete;
}

void cdsl_bstreePrint(bs_treeNode_t** root,cdsl_generic_printer_t prt){
	if(!root)
		return;
	print_r(*root,prt,0);
}

int cdsl_bstreeMaxDepth(bs_treeNode_t** root){
	if(!root || !(*root))
		return 0;
	int max = 0;
	int temp = 0;
	if(max < (temp = cdsl_bstreeMaxDepth(&(*root)->left)))
		max = temp;
	if(max < (temp = cdsl_bstreeMaxDepth(&(*root)->right)))
		max = temp;
	return max + 1;
}


int cdsl_bstreeSize(bs_treeNode_t** root){
	int cnt = 0;
	if(!root || !(*root))
		return 0;
	if((*root))
		cnt = 1;
	if(!(*root)->left && !(*root)->right) return cnt;
	if((*root)->left)
		cnt += cdsl_bstreeSize(&(*root)->left);
	if((*root)->right)
		cnt += cdsl_bstreeSize(&(*root)->right);
	return cnt;
}

static void print_r(bs_treeNode_t* current,cdsl_generic_printer_t prt,int depth){
	if(!current)
		return;
	print_r(current->right,prt,depth + 1);
	print_tab(depth); if(prt) prt(current); printf("{key : %d} @ depth %d\n",current->key,depth);
	print_r(current->left,prt,depth + 1);
}

static void print_tab(int cnt){
	while(cnt--)
		printf("\t");
}
