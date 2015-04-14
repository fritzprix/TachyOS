/*
 * tch_mailq.h
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#ifndef TCH_MAILQ_H_
#define TCH_MAILQ_H_


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct _tch_mailq_karg{
	uint32_t timeout;
	void*    chunk;
}tch_mailq_karg;

extern tch_mailqId tch_mailqInit(tch_mailqId qdest,tch_mailqId qsrc);
extern tchStatus tchk_mailqAlloc(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tchk_mailqFree(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tchk_mailqDeinit(tch_mailqId qid);



#if defined(__cplusplus)
}
#endif

#endif /* TCH_MAILQ_H_ */
