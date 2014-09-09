/*
 * tch_sys.h
 *
 *  Created on: 2014. 9. 6.
 *      Author: innocentevil
 */

#ifndef TCH_SYS_H_
#define TCH_SYS_H_

#if defined(__cplusplus)
extern "C"{
#endif

typedef struct _tch_msgq_karg_t{
	uword_t     msg;
	uint32_t    timeout;
}tch_msgq_karg;

extern tchStatus tch_msgq_kput(tch_msgQue_id,tch_msgq_karg* arg);
extern tchStatus tch_msgq_kget(tch_msgQue_id,tch_msgq_karg* arg);
extern tchStatus tch_msgq_kdestroy(tch_msgQue_id);


typedef struct _tch_mailq_karg{
	uint32_t timeout;
	void*    chunk;
}tch_mailq_karg;

extern tchStatus tch_mailq_kalloc(tch_mailQue_id qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kfree(tch_mailQue_id qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kdestroy(tch_mailQue_id qid,tch_mailq_karg* arg);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_SYS_H_ */
