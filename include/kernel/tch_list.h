/*
 * tch_list.h
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */

#ifndef TCH_LIST_H_
#define TCH_LIST_H_


#if defined(__cplusplus)
extern "C" {
#endif

// Static Initializer for List Node
#define INIT_LIST   {NULL,NULL}
// Decl for Compare Function for priority queue operation
#define DECLARE_COMPARE_FN(fn) int fn(void* prior,void* post)
#define tch_listIsEmpty(lhead)   (((tch_lnode_t*)lhead)->next == NULL)

typedef struct _tch_lnode_t tch_lnode_t;

 struct _tch_lnode_t {
	tch_lnode_t* prev;
	tch_lnode_t* next;
};




extern void tch_listInit(tch_lnode_t* lentry);
extern void tch_listEnqueuePriority(tch_lnode_t* lentry,tch_lnode_t* item,int (*cmp)(void* prior,void* post));
extern void* tch_listDequeue(tch_lnode_t* lentry);
extern void tch_listPutFirst(tch_lnode_t* lentry,tch_lnode_t* item);
extern void tch_listPutLast(tch_lnode_t* lentry,tch_lnode_t* item);
extern void tch_listPush(tch_lnode_t* lentry,tch_lnode_t* item);
extern int tch_listRemove(tch_lnode_t* lentry,tch_lnode_t* item);
extern void* tch_listPop(tch_lnode_t* lentry);
extern int tch_listSize(tch_lnode_t* lentry);
extern int tch_listContain(tch_lnode_t* lentry,tch_lnode_t* item);
extern void tch_listPrint(tch_lnode_t* lentry,void (*printitem)(void* item));

#if defined(__cplusplus)
}
#endif


#endif /* TCH_LIST_H_ */
