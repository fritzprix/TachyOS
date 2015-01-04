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

tch_btree_node* tch_btree_insert(tch_btree_node* root,tch_btree_node* item){
	if(!item)
		return NULL;
	while(root){
		if(root->key < item->key){
			if(!root->right){
				root->right = item;
				return item;
			}
			root = root->right;
		}else{
			if(!root->left){
				root->left = item;
				return item;
			}
			root = root->left;
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
	tch_btree_node* ltr = tch_btree_split(root,key);
	if(!ltr)
		return NULL;
	tch_btree_insert(*root,ltr->left);
	ltr->left = NULL;
	return ltr;
}

int tch_btree_update(tch_btree_node** rbr,tch_btree_node* item){
	tch_btree_node* prev = NULL;
		tch_btree_node* root = *rbr;
		while(root){
			if(root->key == item->key){
				item->right = root->right;
				item->left = root->left;
				if(!prev)
					*rbr = item;
				else{
					if(prev->key < item->key)
						prev->right = item;
					else
						prev->left = item;
				}
				return (1 > 0);
			}
			if(root->key < item->key){
				prev = root;
				root = root->right;
			}else{
				prev = root;
				root = root->left;
			}
		}
		return (1 < 0);
}


tch_btree_node* tch_btree_split(tch_btree_node** rbr,int key){
	tch_btree_node* root = *rbr;
	tch_btree_node* p = NULL;
	while(root){
		if(root->key == key){
			if(!p){
				if(!root->right)
					return NULL;
				*rbr = root->right;
				root->right = NULL;
				return root;
			}else{
				if(p->right == root){
					p->right = root->right;
				}else{
					p->left = root->right;
				}
				root->right = NULL;
				return root;
			}
		}
		p = root;
		if(root->key < key)
			root = root->right;
		else
			root = root->left;
	}
	return NULL;
}
