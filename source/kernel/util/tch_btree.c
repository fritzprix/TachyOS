/*
 * tch_btree.c
 *
 *  Created on: 2014. 9. 12.
 *      Author: innocentevil
 */


#include "tch_btree.h"
#include <stddef.h>


void tch_btreeInit(tch_btree_node* node,int key){
	node->key = key;
	node->left = NULL;
	node->right = NULL;
}

static tch_btree_node* tch_btree_get_rightmost(tch_btree_node* node){
	if(!node) return NULL;
	if(!node->right) return node;
	tch_btree_node* rightmost = tch_btree_get_rightmost(node->right);
	node->right = rightmost->left;
	rightmost->left = node;
	return rightmost;
}

static tch_btree_node* tch_btree_get_leftmost(tch_btree_node* node){
	if(!node) return NULL;
	if(!node->left) return node;
	tch_btree_node* leftmost = tch_btree_get_leftmost(node->left);
	node->left = leftmost->right;
	leftmost->right = node;
	return leftmost;
}


tch_btree_node* tch_btree_insert(tch_btree_node** root,tch_btree_node* item){
	if(!root || !item)
		return NULL;
	if(!*root){
		*root = item;
		return item;
	}
	item->left = NULL;
	item->right = NULL;
	tch_btree_node* current = *root;
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

tch_btree_node* tch_btree_lookup(tch_btree_node* root,int key){
	while(root){
		if(root->key == key)
			return root;
		if(root->key < key)
			root = root->right;
		else
			root = root->left;
	}
	return NULL;
}

tch_btree_node* tch_btree_delete(tch_btree_node** root,int key){

	tch_btree_node* todelete = NULL;
	if(!root || !*root)
		return NULL;
	tch_btree_node** current = root;
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

int tch_btree_size(tch_btree_node* root){
	int cnt = 0;
	if(root)
		cnt = 1;
	if(!root->left && !root->right) return cnt;
	if(root->left)
		cnt += tch_btree_size(root->left);
	if(root->right)
		cnt += tch_btree_size(root->right);
	return cnt;
}


