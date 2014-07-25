/*
 * tch_list.h
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */

#ifndef TCH_LIST_H_
#define TCH_LIST_H_

#include <stdint-gcc.h>

typedef struct _tch_lnode_t {
	void* prev;
	void* next;
}tch_lnode_t;


extern void tch_listInit(tch_lnode_t* lentry);
extern void tch_listEnqueuePriority(tch_lnode_t* lentry,tch_lnode_t* item,int (*cmp)(void* prior,void* post));
extern void* tch_listDequeue(tch_lnode_t* lentry);
extern void tch_listPutFirst(tch_lnode_t* lentry,tch_lnode_t* item);
extern void tch_listPutLast(tch_lnode_t* lentry,tch_lnode_t* item);
extern int tch_listRemove(tch_lnode_t* lentry,tch_lnode_t* item);
extern int tch_listSize(tch_lnode_t* lentry);
extern int tch_listContain(tch_lnode_t* lentry,tch_lnode_t* item);
extern void tch_listPrint(tch_lnode_t* lentry,void (*printitem)(void* item));



#endif /* TCH_LIST_H_ */
