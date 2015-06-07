/*
 * cdsl_bst.h
 *
 *  Created on: 2014. 9. 12.
 *      Author: innocentevil
 */

#ifndef CDSL_BSTREE_H_
#define CDSL_BSTREE_H_

#include "cdsl.h"

#if defined(__cplusplus)
extern "C"{
#endif


typedef struct cdsl_bst_node bs_treeNode_t;

struct cdsl_bst_node {
	bs_treeNode_t* right;
	bs_treeNode_t* left;
	int key;
};



extern void cdsl_bstreeNodeInit(bs_treeNode_t* node,int key);
extern bs_treeNode_t* cdsl_bstreeInsert(bs_treeNode_t** root,bs_treeNode_t* item);
extern bs_treeNode_t* cdsl_bstreeLookup(bs_treeNode_t* root,int key);
extern bs_treeNode_t* cdsl_bstreeDelete(bs_treeNode_t** root,int key);
extern int cdsl_bstreeSize(bs_treeNode_t** root);
extern void cdsl_bstreePrint(bs_treeNode_t** root,cdsl_generic_printer_t prt);
extern int cdsl_bstreeMaxDepth(bs_treeNode_t** root);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_BTREE_H_ */
