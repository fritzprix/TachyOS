/*
 * fmo_thread.c
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */


#include "fmo_thread.h"
#include "fmo_sched.h"






static tchThread_t* _fmo_thread_init(tchThread_t* thread);






/**
 * thread queue functions
 */

/*
void tch_thQue_Init(tchThread_queue* queue){
	queue->tail = NULL;
	queue->entry = NULL;
	queue->cnt = 0;
}

void tch_thQue_enqueue_byprior(tchThread_queue* queue,tchThread_t* thread){
	queue->cnt++;
	tchThread_t* cth = queue->entry;
	if(cth == NULL){
		queue->entry = thread;
		queue->tail = thread;
		queue->cnt++;
		return;
	}
	while(cth->t_next != NULL){
		if(cth->t_prior < thread->t_prior){
			thread->t_prev = cth->t_prev;
			thread->t_next = cth;
			cth->t_prev = thread;
			if(thread->t_prev == NULL){
				queue->entry = thread;
			}else{
				thread->t_prev->t_next = thread;
			}
			return;
		}
		cth = cth->t_next;
	}
	if(cth->t_prior < thread->t_prior){
		thread->t_prev = cth->t_prev;
		thread->t_next = cth;
		cth->t_prev = thread;
		if(thread->t_prev == NULL){
			queue->entry = thread;
		}else{
			thread->t_prev->t_next = thread;
		}
		return;
	}
	//last element in the queue
	cth->t_next = thread;
	thread->t_prev = cth;
	thread->t_next = NULL;
	queue->tail = thread;
}

void tch_thQue_enqueue_byTo(tchThread_queue* queue,tchThread_t* thread){
	queue->cnt++;
	tchThread_t* cth = queue->entry;
		if(cth == NULL){
			/// new thread is inserted queue entry point
			thread->t_next = NULL;
			thread->t_prev = NULL;
			queue->entry = thread;
			queue->tail = thread;
			queue->cnt++;
			return;
		}
		while(cth->t_next != NULL){
			if(cth->t_to > thread->t_to){
				thread->t_prev = cth->t_prev;
				thread->t_next = cth;
				cth->t_prev = thread;
				if(thread->t_prev == NULL){
					queue->entry = thread;
				}else{
					thread->t_prev->t_next = thread;
				}
				return;
			}
			cth = cth->t_next;
		}
		if(cth->t_to > thread->t_to){
			thread->t_prev = cth->t_prev;
			thread->t_next = cth;
			cth->t_prev = thread;
			if(thread->t_prev == NULL){
				queue->entry = thread;
			}else{
				thread->t_prev->t_next = thread;
			}
			return;
		}
		//last element in the queue
		cth->t_next = thread;
		thread->t_prev = cth;
		thread->t_next = NULL;
		queue->tail = thread;
}

void tch_thQue_enqueueAhead(tchThread_queue* queue,tchThread_t* thread){
	thread->t_next = queue->entry->t_next;
	thread->t_prev = NULL;
	queue->entry = thread;
	queue->cnt++;
}

void tch_thQue_enqueueTail(tchThread_queue* queue,tchThread_t* thread){
	thread->t_next = NULL;
	queue->tail->t_next = thread;
	thread->t_prev = queue->tail;
	queue->tail = thread;
	queue->cnt++;
}

tchThread_t* tch_thQue_dequeue(tchThread_queue* queue){
	queue->cnt--;
	tchThread_t* hth = queue->entry;
	queue->entry = hth->t_next;
	queue->entry->t_prev = NULL;
	hth->t_prev = NULL;
	hth->t_next = NULL;
	return hth;
}

int tch_thQue_remove(tchThread_queue* queue,tchThread_t* thread){
	tchThread_t* thr = queue->entry;
	if(thr == NULL){
		return ERROR;
	}
	queue->cnt--;
	while(thr->t_next != NULL){
		if(thr == thread){
			if(thread->t_prev != NULL){
				thread->t_prev->t_next = thread->t_next;
			}
			thread->t_next->t_prev = thread->t_prev;
			return SUCCESS;
		}
		thr = thr->t_next;
	}
	return ERROR;
}


*/


/**
 * tchthread function
 */
tchThread_t* tchThread_create(uint32_t* th_spb,uint32_t size,void* (*fn)(void*) ,uint32_t t_prior,void* arg,const char* name){
	uint32_t* spt = ((uint32_t*)th_spb + size);
	tchThread_t* thread = ((tchThread_t*)spt - 1);
	thread->t_id = (uint32_t)thread;                   // allocate thread structure from the stack top
	thread->t_tctx.R13 = ((arm_sbrn_cntx*)thread - 1); // allocate context stack structure for saving cntxt switching point
	thread->t_prior = t_prior;                              // бщ Initialize Thread Data Structure
	thread->t_svd_prior = t_prior;
	thread->t_fn = fn;
	thread->t_id = (uint32_t) thread;
	thread->t_arg = arg;
#ifdef THR_TRACE_ENABLE
	thread->t_traceIdx = 0;
#endif
	if(name){
		thread->t_name = name;
	}else{
		thread->t_name = "Unnamed";
	}

	return _fmo_thread_init(thread);
}


tchThread_t* _fmo_thread_init(tchThread_t* thread){
	/*
	thread->t_next = NULL;
	thread->t_prev = NULL;*/
	thread->t_listNode.next = NULL;
	thread->t_listNode.prev = NULL;
	tch_genericQue_Init(&thread->t_joinQ);
	thread->t_tslot = THREAD_TIME_SLOT;
	thread->t_lckCnt = 0;
	thread->t_to = 0;
	thread->t_status = 0;
	SET_THREAD_STATUS(thread,THREAD_STATUS_DEACTIVE);
	return thread;
}


BOOL tchThread_start(tchThread_t* thread){
	sched_startThread(thread);
	return TRUE;
}

BOOL tchThread_sleep(uint32_t milliSec){
	sched_pendCurrentThreadTimeout(milliSec);
	return TRUE;
}


BOOL tchThread_wait(tch_genericList_queue_t* queue){
	sched_pendCurrentThreadInWaitQueue(queue,NULL);
	return TRUE;
}

BOOL tchThread_wake(tch_genericList_queue_t* queue){
	sched_wakeThreadFromQueue(queue);
	return TRUE;
}

BOOL tchThread_join(tchThread_t* jtarget){
	sched_pendCurrentJoinThread(jtarget);
	return TRUE;
}

tchThread_t* tchThread_getCurrent(){
	return sched_getCurrentThread();
}
