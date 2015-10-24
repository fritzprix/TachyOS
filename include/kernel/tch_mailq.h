/*
 * tch_mailq.h
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#ifndef TCH_MAILQ_H_
#define TCH_MAILQ_H_



#include "tch_kobj.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_mailqCb {
	tch_kobj     	 	__obj;
	uint32_t      	 	bstatus;         // state
	uint32_t      	 	bsz;
	uint32_t	  	 	qlen;
	uint32_t	 	 	pidx;
	uint32_t	 	 	gidx;
	uint32_t			qupdated;
	cdsl_dlistNode_t 	pwq;
	cdsl_dlistNode_t 	gwq;
	void**				queue;
	tch_mpoolId   		bpool;
	cdsl_dlistNode_t  	allocwq;
}tch_mailqCb;

typedef struct _tch_mailq_karg{
	uint32_t timeout;
	void*    chunk;
}tch_mailq_karg;

extern tch_kernel_service_mailQ MailQ_IX;
extern tchStatus tch_mailqInit(tch_mailqCb* qcb,uint32_t sz,uint32_t qlen);
extern tchStatus tch_mailqDeinit(tch_mailqCb* qcb);



#if defined(__cplusplus)
}
#endif

#endif /* TCH_MAILQ_H_ */
