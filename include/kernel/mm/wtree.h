/*
 * wtree.h
 *
 *  Created on: Jun 13, 2015
 *      Author: innocentevil
 */

#ifndef __WTREE_H
#define __WTREE_H

#include <stdint.h>
#include <stddef.h>

typedef struct wtreeNode wtreeNode_t;
typedef struct wtreeRoot wtreeRoot_t;
typedef int (*wtreeMerge)(wtreeNode_t* a,wtreeNode_t* b);

struct wtreeNode {
	wtreeNode_t *left,*right;
	uint32_t base;
	uint32_t span;
};

struct wtreeRoot {
	wtreeNode_t*	entry;
	uint32_t		ext_gap;
};

extern void wtreeRootInit(wtreeRoot_t* root,uint32_t ext_gap);
extern void wtreeNodeInit(wtreeNode_t* node,uint32_t base,uint32_t span);
extern void wtreeInsert(wtreeRoot_t* root,wtreeNode_t* item);
extern wtreeNode_t* wtreeRetrive(wtreeRoot_t* root,uint32_t* span);
extern wtreeNode_t* wtreeDeleteRightMost(wtreeRoot_t*root);
extern size_t wtreeTotalSpan(wtreeRoot_t* root);

extern void wtreePrint(wtreeRoot_t* root);

#endif
