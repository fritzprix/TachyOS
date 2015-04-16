/*
 * tch_ltree.h
 *
 *  Created on: 2014. 9. 26.
 *      Author: innocentevil
 */

#ifndef TCH_LTREE_H_
#define TCH_LTREE_H_

#include "tch_list.h"
#include "tch_btree.h"

#if defined(__cplusplus)
extern "C"{
#endif


#define INIT_LTREE                 {INIT_BTREE((0xFFFFFF >> 1)),INIT_LIST}
#define tch_ltreeIsEmpty(ltree)    (((tch_ltree_node*)ltree)->ln.next == NULL)
typedef struct tch_ltree_node_t {
	tch_btree_node trn;
	tch_lnode    ln;
}tch_ltree_node;

extern void tch_ltreeInit(tch_ltree_node* root,int key);
extern tch_ltree_node* tch_ltreeInsert(tch_ltree_node* root, tch_ltree_node* item);
extern tch_ltree_node* tch_ltreeLookup(tch_ltree_node* root, int key);
extern tch_ltree_node* tch_ltreeRemove(tch_ltree_node** rtp,int key);
extern tch_ltree_node* tch_ltreeRemoveHead(tch_ltree_node** rtp);
extern tch_ltree_node* tch_ltreeRemoveTail(tch_ltree_node** rtp);



#if defined(__cplusplus)
}
#endif


#endif /* TCH_LTREE_H_ */
