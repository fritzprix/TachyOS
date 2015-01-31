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




extern void tch_schedInit(void* _systhread);
extern BOOL tch_schedIsPreemptable(tch_threadId nth);
extern tch_threadId tch_schedGetRunningThread();

extern void tch_schedStart(tch_threadId thr_id);
extern void tch_schedReady(tch_threadId thr_id);
extern void tch_schedSleep(uint32_t timeout,tch_timeunit tu,tch_thread_state nextState);
extern void tch_schedTerminate(tch_threadId thread, int result);
extern void tch_schedDestroy(tch_threadId thread,int result);
extern void tch_schedSuspend(tch_thread_queue* wq,uint32_t timeout);
extern int tch_schedResume(tch_thread_queue* wq,tch_threadId thread,tchStatus res,BOOL preemt);   // resume specific thread in wait queue
extern BOOL tch_schedResumeM(tch_thread_queue* wq,int cnt,tchStatus res,BOOL preemt);
extern void tch_schedYieldThread(uint32_t timeout,tch_thread_state nextState);

extern BOOL tch_schedIsEmpty();
extern void tch_schedUpdate(void);


#endif /* TCH_SCHED_H_ */
