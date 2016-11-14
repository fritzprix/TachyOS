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

typedef struct owtreeNode owtreeNode_t;
typedef struct owtreeRoot owtreeRoot_t;
typedef int (*wtreeMerge)(owtreeNode_t* a,owtreeNode_t* b);

struct owtreeNode {
	owtreeNode_t *left,*right;
	uint32_t base;
	uint32_t span;
};

struct owtreeRoot {
	owtreeNode_t*	entry;
	uint32_t		ext_gap;
};

extern void owtreeRootInit(owtreeRoot_t* root,uint32_t ext_gap);
extern void owtreeNodeInit(owtreeNode_t* node,uint32_t base,uint32_t span);
extern void owtreeInsert(owtreeRoot_t* root,owtreeNode_t* item);
extern owtreeNode_t* owtreeRetrive(owtreeRoot_t* root,uint32_t* span);
extern owtreeNode_t* owtreeDeleteRightMost(owtreeRoot_t*root);
extern size_t owtreeTotalSpan(owtreeRoot_t* root);

extern void owtreePrint(owtreeRoot_t* root);

#endif
