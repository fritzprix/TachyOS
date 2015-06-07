/*
 * cdsl_rbtree.h
 *
 *  Created on: 2015. 5. 14.
 *      Author: innocentevil
 */

#ifndef CDSL_RBTREE_H_
#define CDSL_RBTREE_H_

#include "cdsl.h"
#include "cdsl_rbtree.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct rb_treeNode rb_treeNode_t;

struct rb_treeNode {
	rb_treeNode_t 	*left,*right;
	unsigned		color : 1;
	unsigned		key   : 31;
} __attribute__((packed));


extern void cdsl_rbtreeNodeInit(rb_treeNode_t* item,int key);
extern BOOL cdsl_rbtreeInsert(rb_treeNode_t** root,rb_treeNode_t* item);
extern rb_treeNode_t* cdsl_rbtreeDelete(rb_treeNode_t** root,int key);
extern int cdsl_rbtreeSize(rb_treeNode_t** root);
extern void cdsl_rbtreePrint(rb_treeNode_t** root);
extern int cdsl_rbtreeMaxDepth(rb_treeNode_t** root);


#if defined(__cplusplus)
}
#endif

#endif /* CDSL_RBTREE_H_ */
