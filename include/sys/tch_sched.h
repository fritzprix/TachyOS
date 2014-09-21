/*
 * tch_sched.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 28.
 *      Author: innocentevil
 */

#ifndef TCH_SCHED_H_
#define TCH_SCHED_H_
/**
 *  Interface for Scheduler
 *  - Scheduler manage ready queue and thread life cycle
 *  - start new thread from kernel mode
 *  - put thread into ready queue (ready and execution)
 *  - put thread into wait queue (wait / wake)
 *  - put thread into kernel timer event queue (sleep)
 *  - remove thread from ready queue or terminate thread
 *
 */


#define SCHED_THREAD_ALL             ((uint32_t) -1)
/***
 *   Initialize Scheduler
 *    - typically called from kernel initialize routine
 *    - initialize thread queue
 */
extern void tch_schedInit(void* arg);

/**
 *   Return true if new thread has higher priority than current one
 *   @param nth : new thread
 */
extern BOOL tch_schedIsPreemptable(tch_threadId nth);

/***.
 *  start new thread
 */
extern void tch_schedStartThread(tch_threadId thr_id);
/***
 *  return current active thread
 */
extern tch_threadId tch_schedGetRunningThread();

/***
 *  put thread into ready queue of scheduler rather than starting immediately
 *  this can be ivnoked from ISR
 */
extern void tch_schedReady(tch_threadId thr_id);


/**
 *  suspend thread for given amount of time
 */
extern void tch_schedSleep(uint32_t timeout,tch_thread_state nextState);

/**
 *
 */
extern void tch_schedSuspend(tch_thread_queue* wq,uint32_t timeout);

extern int tch_schedResumeThread(tch_thread_queue* wq,tch_threadId thread,tchStatus res,BOOL preemt);   // resume specific thread in wait queue

extern BOOL tch_schedResumeM(tch_thread_queue* wq,int cnt,tchStatus res,BOOL preemt);


/**
 *
 */
extern tchStatus tch_schedCancelTimeout(tch_threadId thread);

extern void tch_schedTerminate(tch_threadId thread, int result);


/***
 *  wait other thread is terminated
 */
extern BOOL tch_schedJoin(tch_threadId thr_id);


#endif /* TCH_SCHED_H_ */
