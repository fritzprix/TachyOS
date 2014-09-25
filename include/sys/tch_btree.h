/*
 * tch_btree.h
 *
 *  Created on: 2014. 9. 12.
 *      Author: innocentevil
 */

#ifndef TCH_BTREE_H_
#define TCH_BTREE_H_

#if defined(__cplusplus)
extern "C"{
#endif


typedef struct _tch_btree_node_t tch_btree_node;

struct _tch_btree_node_t{
	tch_btree_node* right;
	tch_btree_node* left;
	int key;
};



extern void tch_btreeInit(tch_btree_node* node,int key);
extern tch_btree_node* tch_btree_insert(tch_btree_node* root,tch_btree_node* item);
extern tch_btree_node* tch_btree_lookup(tch_btree_node* root,int key);
extern tch_btree_node* tch_btree_delete(tch_btree_node** root,int key);
extern int tch_btree_update(tch_btree_node** root,tch_btree_node* item); //
extern tch_btree_node* tch_btree_split(tch_btree_node** rbr,int key);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_BTREE_H_ */
