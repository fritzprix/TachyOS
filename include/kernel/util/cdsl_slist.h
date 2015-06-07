/*
 * cdsl_slist.h
 *	\brief singly linked list library
 *  Created on: 2015. 5. 31.
 *      Author: innocentevil
 */

#ifndef CDSL_SLIST_H_
#define CDSL_SLIST_H_

#include "cdsl.h"



#ifndef DECLARE_COMPARE_FN
#define DECLARE_COMPARE_FN(fn) void* fn(void* a,void* b)
#endif

#define cdsl_slistIsEmpty(node) (((cdsl_slistNode_t*) node)->next == NULL)

typedef struct cdsl_slnode cdsl_slistNode_t;
typedef void* (*cdsl_slistPriorityRule)(void* a, void* b);
struct cdsl_slnode {
	cdsl_slistNode_t* next;
};

extern void cdsl_slistInit(cdsl_slistNode_t* lentry);


extern void cdsl_slistEnqueuePriority(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item,cdsl_slistPriorityRule rule);
extern void cdsl_slistInsertAfter(cdsl_slistNode_t* ahead,cdsl_slistNode_t* item);
extern cdsl_slistNode_t* cdsl_slistDequeue(cdsl_slistNode_t* lentry);

extern void cdsl_slistPutHead(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item);
extern void cdsl_slistPutTail(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item);
extern cdsl_slistNode_t* cdsl_slistGetHead(cdsl_slistNode_t* lentry);
extern cdsl_slistNode_t* cdsl_slistGetTail(cdsl_slistNode_t* lentry);

extern BOOL cdsl_slistRemove(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item);
extern int cdsl_slistSize(cdsl_slistNode_t* lentry);
extern BOOL cdsl_slistContain(cdsl_slistNode_t* lentry,cdsl_slistNode_t* item);
extern void cdsl_slistPrint(cdsl_slistNode_t* lentry,cdsl_generic_printer_t prt);



#endif /* CDSL_SLIST_H_ */
