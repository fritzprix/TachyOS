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

extern tchStatus tch_mailq_kalloc(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kfree(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kdestroy(tch_mailqId qid,tch_mailq_karg* arg);



#if defined(__cplusplus)
}
#endif

#endif /* TCH_MAILQ_H_ */
