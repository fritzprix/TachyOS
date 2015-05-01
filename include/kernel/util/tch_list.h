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

#define INIT_LIST   {NULL,NULL}


#define DECLARE_COMPARE_FN(fn) void* fn(void* prior,void* post)
#define tch_listIsEmpty(lhead)   (((tch_lnode*)lhead)->next == NULL)

typedef void* (tch_listPriorityRule)(void*,void*);
typedef struct _tch_lnode_t tch_lnode;
 struct _tch_lnode_t {
	tch_lnode* prev;
	tch_lnode* next;
};

extern void tch_listInit(tch_lnode* lentry);
extern void tch_listEnqueueWithPriority(tch_lnode* lentry,tch_lnode* item,tch_listPriorityRule rule);
extern tch_lnode* tch_listDequeue(tch_lnode* lentry);
extern void tch_listPutHead(tch_lnode* lentry,tch_lnode* item);
extern void tch_listPutTail(tch_lnode* lentry,tch_lnode* item);
extern tch_lnode* tch_listGetHead(tch_lnode* lentry);
extern tch_lnode* tch_listGetTail(tch_lnode* lentry);
extern int tch_listRemove(tch_lnode* lentry,tch_lnode* item);
extern int tch_listSize(tch_lnode* lentry);
extern int tch_listContain(tch_lnode* lentry,tch_lnode* item);
extern void tch_listPrint(tch_lnode* lentry,void (*printitem)(void* item));

#if defined(__cplusplus)
}
#endif


#endif /* TCH_LIST_H_ */
