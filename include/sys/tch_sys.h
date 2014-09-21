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

extern tchStatus tch_mailq_kalloc(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kfree(tch_mailqId qid,tch_mailq_karg* arg);
extern tchStatus tch_mailq_kdestroy(tch_mailqId qid,tch_mailq_karg* arg);

extern tchStatus tch_async_kwait(tch_asyncId async,void* async_req,void* task_queue);
extern tchStatus tch_async_knotify(tch_asyncId async,void* res);
extern tchStatus tch_async_kdestroy(tch_asyncId async);


#if defined(__cplusplus)
}
#endif


#endif /* TCH_SYS_H_ */
