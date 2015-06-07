/*
 * tch_dlist.h
 *
 * \brief doubly linked list library (can be used as stack or queue)
 *
 *
 *  Created on: 2014. 7. 25.
 *      Author: innocentevil
 */

#ifndef CDSL_DLIST_H_
#define CDSL_DLIST_H_

#include "cdsl.h"

#if defined(__cplusplus)
extern "C" {
#endif


#ifndef DECLARE_COMPARE_FN
#define DECLARE_COMPARE_FN(fn) void* fn(void* a,void* b)
#endif

#define cdsl_dlistIsEmpty(lhead)   (((cdsl_dlistNode_t*) lhead)->next == NULL)
typedef void* (*cdsl_dlistPriorityRule)(void* a,void* b);		// should return larger or prior one between two
typedef struct cdsl_dlnode cdsl_dlistNode_t;
 struct cdsl_dlnode {
	cdsl_dlistNode_t* prev;
	cdsl_dlistNode_t* next;
};

extern void cdsl_dlistInit(cdsl_dlistNode_t* lentry);
/**
 *  Queue like interface
 */
extern void cdsl_dlistEnqueuePriority(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item,cdsl_dlistPriorityRule rule);
extern void cdsl_dlistInsertAfter(cdsl_dlistNode_t* ahead,cdsl_dlistNode_t* item);
extern cdsl_dlistNode_t* cdsl_dlistDequeue(cdsl_dlistNode_t* lentry);

/**
 *  Stack like interface
 */
extern void cdsl_dlistPutHead(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item);
extern void cdsl_dlistPutTail(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item);
extern cdsl_dlistNode_t* cdsl_dlistGetHead(cdsl_dlistNode_t* lentry);
extern cdsl_dlistNode_t* cdsl_dlistGetTail(cdsl_dlistNode_t* lentry);


extern BOOL cdsl_dlistRemove(cdsl_dlistNode_t* item);
extern int cdsl_dlistSize(cdsl_dlistNode_t* lentry);
extern BOOL cdsl_dlistContain(cdsl_dlistNode_t* lentry,cdsl_dlistNode_t* item);
extern void cdsl_dlistPrint(cdsl_dlistNode_t* lentry,cdsl_generic_printer_t prt);

#if defined(__cplusplus)
}
#endif


#endif /* TCH_LIST_H_ */
