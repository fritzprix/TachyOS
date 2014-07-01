/*
 * tch_sched.h
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
extern BOOL tch_schedIsPreemtable(tch_thread_id nth);

/***.
 *  start new thread immediately
 */
extern osStatus tch_schedStartThread(tch_thread_id thr_id);
/***
 *  return current active thread
 */
extern tch_thread_id tch_schedGetRunningThread();

/***
 *  put thread into ready queue of scheduler rather than starting immediately
 *  this can be ivnoked from ISR
 */
extern void tch_schedScheduleToReady(tch_thread_id thr_id);

/**
 *  suspend thread for given amount of time
 */
extern osStatus tch_schedScheduleToSuspend(uint32_t timeout);


/***
 *  wait other thread is terminated
 */
extern BOOL tch_schedJoin(tch_thread_id thr_id);

#endif /* TCH_SCHED_H_ */
