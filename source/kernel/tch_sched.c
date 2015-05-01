/*
 * tch_sched.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_list.h"
#include "tch_sched.h"
#include "tch_thread.h"
#include "tch_mem.h"




/* =================  private internal function declaration   ========================== */

#define getListNode(th_id)    ((tch_lnode*) th_id)
/***
 *  Scheduling policy Implementation
 */
static DECLARE_COMPARE_FN(tch_schedReadyQRule);
static DECLARE_COMPARE_FN(tch_schedWqRule);

/***
 *  Invoked when new thread start
 */
static inline void tch_schedInitKernelThread(tch_threadId thr)__attribute__((always_inline));
static BOOL tch_schedIsPreemptable(tch_thread_kheader* nth);


static tch_thread_queue tch_readyQue;        ///< thread wait to become running state
tch_thread_uheader* tch_currentThread;




void tch_schedInit(void* init_thread){

	/**
	 *  Initialize ready & pend queue
	 *   - ready queue : waiting queue in which active thread is waiting to occupy cpu run time (to be executed)
	 *   - pend queue : waiting queue in which suspended thread is waiting to be re activated
	 *
	 */
	tch_listInit((tch_lnode*)&tch_readyQue);
	tch_schedInitKernelThread(init_thread);

	while(TRUE){     // Unreachable Code. Ensure to protect from executing when undetected schedulr failure happens
		__WFI();
	}
}


/** Note : should not called any other program except kernel mode program
 *  @brief start thread based on its priority (thread can be started in preempted way)
 */
extern void tchk_schedThreadStart(tch_threadId thread){
	tch_thread_uheader* thr = (tch_thread_uheader*) thread;
	if(tch_schedIsPreemptable(thr->t_kthread)){
		tch_listEnqueueWithPriority((tch_lnode*)&tch_readyQue,getListNode(tch_currentThread->t_kthread),tch_schedReadyQRule);			///< put current thread in ready queue
		getListNode(thr->t_kthread)->prev = (tch_lnode*) tch_currentThread->t_kthread; 	                                                            ///< set new thread as current thread
		tch_currentThread = thr;
		((tch_thread_kheader*)getListNode(tch_currentThread->t_kthread)->prev)->t_state = READY;
		tch_port_jmpToKernelModeThread((uaddr_t) tch_port_switchContext,(uword_t)thread,(uword_t) getListNode(thread)->prev,tchOK);
	}else{
		thr->t_kthread->t_state = READY;
		tchk_kernelSetResult(tch_currentThread,tchOK);
		tch_listEnqueueWithPriority((tch_lnode*)&tch_readyQue,getListNode(thr->t_kthread),tch_schedReadyQRule);
	}
}


/**
 * put new thread into ready queue
 */
void tchk_schedThreadReady(tch_threadId thread){
	tch_thread_uheader* thr = (tch_thread_uheader*) thread;
	thr->t_kthread->t_state = READY;
	tch_listEnqueueWithPriority((tch_lnode*)&tch_readyQue,getListNode(thr->t_kthread),tch_schedReadyQRule);
}

/** Note : should not called any other program except kernel mode program
 *  make current thread sleep for specified amount of time (yield cpu time)
 */
void tchk_schedThreadSleep(uint32_t timeout,tch_timeunit tu,tch_threadState nextState){
	tch_thread_kheader* nth = NULL;
	if(timeout == tchWaitForever)
		return;
	tchk_systimeSetTimeout(tch_currentThread,timeout,tu);
	nth = (tch_thread_kheader*) tch_listDequeue((tch_lnode*)&tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	tchk_kernelSetResult(tch_currentThread,tchEventTimeout);
	getListNode(nth)->prev = (tch_lnode*) getThreadKHeader(tch_currentThread);
	getThreadKHeader(tch_currentThread)->t_state = nextState;
	tch_currentThread = nth->t_uthread;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,nth->t_uthread->t_kRet);
}


void tchk_schedThreadSuspend(tch_thread_queue* wq,uint32_t timeout){
	tch_thread_kheader* nth = NULL;
	if(timeout != tchWaitForever){
		tchk_systimeSetTimeout(tch_currentThread,timeout,mSECOND);
	}
	tch_listEnqueueWithPriority((tch_lnode*) wq,&getThreadKHeader(tch_currentThread)->t_waitNode,tch_schedWqRule);
	nth = (tch_thread_kheader*) tch_listDequeue((tch_lnode*) &tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	getListNode(nth)->prev = (tch_lnode*) getThreadKHeader(tch_currentThread);
	getThreadKHeader(tch_currentThread)->t_state = WAIT;
	getThreadKHeader(tch_currentThread)->t_waitQ = (tch_lnode*) wq;
	tch_currentThread = nth->t_uthread;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,nth->t_uthread->t_kRet);
}


int tch_schedThreadResume(tch_thread_queue* wq,tch_threadId thread,tchStatus res,BOOL preemt){
	tch_thread_kheader* nth = getThreadKHeader(thread);
	if(tch_listIsEmpty(wq)){
		return FALSE;
	}
	if(!tch_listRemove((tch_lnode*) wq,&nth->t_waitNode)){    // try remove given thread from wait Q
		return FALSE;                                  // if thread is not in the wait Q return
	}
	nth->t_waitQ = NULL;
	if(nth->t_to)
		tch_systimeCancelTimeout(thread);
	tchk_kernelSetResult(nth,res);                                // <- check really necessary
	if(tch_schedIsPreemptable(nth) && preemt){
		tchk_schedThreadReady(tch_currentThread);                       // put current thread in ready queue for preemption
		getListNode(nth)->prev = getListNode(getThreadKHeader(tch_currentThread)); // set current thread as previous node of new thread
		tch_currentThread = nth->t_uthread;                                 // set new thread as current thread
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,res);    // go to switch context
		return TRUE;
	}
	tchk_schedThreadReady(nth->t_uthread);
	return TRUE;

}

BOOL tchk_schedThreadResumeM(tch_thread_queue* wq,int cnt,tchStatus res,BOOL preemt){
	tch_thread_kheader* nth = NULL;
	tch_thread_kheader* tpreempt = NULL;
	if(tch_listIsEmpty(wq))
		return FALSE;
	while((!tch_listIsEmpty(wq)) && (cnt--)){
		nth = (tch_thread_kheader*) ((tch_lnode*) tch_listDequeue((tch_lnode*) wq) - 1);
		nth->t_waitQ = NULL;
		if(nth->t_to)
			tch_systimeCancelTimeout(nth->t_uthread);
		tchk_kernelSetResult(nth->t_uthread,res);
		if(tch_schedIsPreemptable(nth) && preemt){
			tpreempt = nth;
		}else{
			tchk_schedThreadReady(nth->t_uthread);
		}
	}
	if(tpreempt && preemt){
		tpreempt->t_state = RUNNING;
		tchk_schedThreadReady(tch_currentThread);
		getListNode(tpreempt)->prev = getListNode(getThreadKHeader(tch_currentThread));
		tch_currentThread = tpreempt->t_uthread;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)tpreempt,(uint32_t)getListNode(tpreempt)->prev,res);        ///< is new thread has higher priority, switch context and caller thread will get 'osOk' as a return value
	}
	return TRUE;
}

void tch_schedThreadUpdate(void){
	tch_thread_kheader* nth = NULL;
	tch_thread_kheader* cth = tch_currentThread->t_kthread;
	if(tch_listIsEmpty(&tch_readyQue) || !tch_schedIsPreemptable((tch_thread_kheader*) tch_readyQue.thque.next))
		return;
	nth = (tch_thread_kheader*)tch_listDequeue((tch_lnode*)&tch_readyQue);
	tch_listEnqueueWithPriority((tch_lnode*) &tch_readyQue,getListNode(cth),tch_schedReadyQRule);
	getListNode(nth)->prev = (tch_lnode*) cth;
	tch_currentThread = nth->t_uthread;
	getThreadKHeader(getListNode(nth)->prev)->t_state = SLEEP;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t) nth,(uint32_t)getListNode(nth)->prev,nth->t_uthread->t_kRet);   // switch context without change
}

BOOL tch_schedIsEmpty(){
	return tch_listIsEmpty(&tch_readyQue);
}

static BOOL tch_schedIsPreemptable(tch_thread_kheader* nth){
	if(!nth)
		return FALSE;
	return tch_schedReadyQRule(nth,getThreadKHeader(tch_currentThread)) || ((getThreadKHeader(tch_currentThread)->t_tslot > 10) && (nth->t_prior != Idle));
}

void tchk_schedThreadTerminate(tch_threadId thread, int result){
	tch_thread_kheader* jth = 0;
	tch_thread_kheader* thr = ((tch_thread_uheader*) thread)->t_kthread;
	thr->t_state = TERMINATED;															/// change state of thread to terminated
	while(!tch_listIsEmpty(&thr->t_joinQ)){
		jth = (tch_thread_kheader*)((tch_lnode*) tch_listDequeue(&thr->t_joinQ) - 1);	/// dequeue thread(s) waiting for termination of this thread
		jth->t_waitQ = NULL;
		tch_listEnqueueWithPriority((tch_lnode*)&tch_readyQue,(tch_lnode*) jth,tch_schedReadyQRule);
		tchk_kernelSetResult(jth,result);
	}
	jth = (tch_thread_kheader*) tch_listDequeue((tch_lnode*) &tch_readyQue);									/// get new thread from readyque
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)jth,(uint32_t)thr,jth->t_uthread->t_kRet);
}

void tch_schedThreadDestroy(tch_threadId thread,int result){
	/// manipulated exc stack to return atexit
	tch_thread_uheader* thr = (tch_thread_uheader*) thread;
	if(thread == tch_currentThread){
		tch_port_jmpToKernelModeThread(__tchk_thread_atexit,(uint32_t)thread,result,0);
	}else{
		tchk_threadInvalidate(thread,result);
	}
}


static inline void tch_schedInitKernelThread(tch_threadId init_thr){
	tch_currentThread = init_thr;
	tch_thread_kheader* thr_p = (tch_thread_kheader*) getThreadKHeader(init_thr);
	tch_port_setThreadSP((uint32_t)thr_p->t_ctx);
#ifdef MFEATURE_HFLOAT
		float _force_fctx = 0.1f;
		_force_fctx += 0.1f;
#endif
	thr_p->t_state = RUNNING;
	int result = getThreadHeader(init_thr)->t_fn(getThreadHeader(init_thr)->t_arg);
	tch_port_enterSv(SV_THREAD_TERMINATE,(uint32_t) thr_p,result);
}

static DECLARE_COMPARE_FN(tch_schedReadyQRule){
	return ((tch_thread_kheader*) prior)->t_prior > ((tch_thread_kheader*) post)->t_prior?  prior : post;
}

static DECLARE_COMPARE_FN(tch_schedWqRule){
	return NULL;
}

