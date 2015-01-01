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

#define getListNode(th_id)    ((tch_lnode_t*) th_id)

/***
 *  Scheduling policy Implementation
 */
static DECLARE_COMPARE_FN(tch_schedReadyQRule);
static DECLARE_COMPARE_FN(tch_schedWqRule);

/***
 *  Invoked when new thread start
 */
static inline void tch_schedInitKernelThread(tch_threadId thr)__attribute__((always_inline));


static tch_thread_queue tch_readyQue;        ///< thread wait to become running state
tch_thread_header* tch_currentThread;






void tch_schedInit(void* _systhread){

	/**
	 *  Initialize ready & pend queue
	 *   - ready queue : waiting queue in which active thread is waiting to occupy cpu run time (to be executed)
	 *   - pend queue : waiting queue in which suspended thread is waiting to be re activated
	 *
	 */
	tch_listInit((tch_lnode_t*)&tch_readyQue);

	tch_currentThread = _systhread;
	tch_schedInitKernelThread(tch_currentThread);

	while(TRUE){     // Unreachable Code. Ensure to protect from executing when undetected schedulr failure happens
		__WFI();
	}
}


/** Note : should not called any other program except kernel mode program
 *  @brief start thread based on its priority (thread can be started in preempted way)
 */
void tch_schedStartThread(tch_threadId nth){
	tch_thread_header* thr_p = nth;
	if(tch_schedIsPreemptable(nth)){
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(tch_currentThread),tch_schedReadyQRule);			///< put current thread in ready queue
		getListNode(nth)->prev = (tch_lnode_t*) tch_currentThread; 	                                                            ///< set new thread as current thread
		tch_currentThread = nth;
		getThreadHeader(tch_currentThread)->t_state = RUNNING;
		getThreadHeader(getListNode(tch_currentThread)->prev)->t_state = READY;
		tch_port_jmpToKernelModeThread((uaddr_t) tch_port_switchContext,(uword_t)nth,(uword_t) getListNode(nth)->prev,tchOK);
	}else{
		getThreadHeader(nth)->t_state = READY;
		tch_kernelSetResult(tch_currentThread,tchOK);
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(nth),tch_schedReadyQRule);
	}
}


/**
 * put new thread into ready queue
 */
void tch_schedReadyThread(tch_threadId th){
       	tch_thread_header* thr_p = th;
	getThreadHeader(th)->t_state = READY;
	tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(thr_p),tch_schedReadyQRule);
}

/** Note : should not called any other program except kernel mode program
 *  make current thread sleep for specified amount of time (yield cpu time)
 */
void tch_schedSleepThread(uint32_t timeout,tch_thread_state nextState){
	tch_threadId nth = 0;
	if(timeout == osWaitForever)
		return;
	tch_systimeSetTimeout(tch_currentThread,timeout);
	nth = tch_listDequeue((tch_lnode_t*)&tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	tch_kernelSetResult(tch_currentThread,tchEventTimeout);
	getListNode(nth)->prev = (tch_lnode_t*) tch_currentThread;
	getThreadHeader(nth)->t_state = RUNNING;
	getThreadHeader(tch_currentThread)->t_state = nextState;
	tch_currentThread = nth;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,getThreadHeader(nth)->t_kRet);
}


void tch_schedSuspendThread(tch_thread_queue* wq,uint32_t timeout){
	tch_threadId nth = 0;
	if(timeout != osWaitForever){
		tch_systimeSetTimeout(tch_currentThread,timeout);
	}
	tch_listEnqueuePriority((tch_lnode_t*) wq,&getThreadHeader(tch_currentThread)->t_waitNode,tch_schedWqRule);
	nth = tch_listDequeue((tch_lnode_t*) &tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,tchErrorOS);
	getListNode(nth)->prev = (tch_lnode_t*) tch_currentThread;
	getThreadHeader(nth)->t_state = RUNNING;
	getThreadHeader(tch_currentThread)->t_state = WAIT;
	getThreadHeader(tch_currentThread)->t_waitQ = (tch_lnode_t*) wq;
	tch_currentThread = nth;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,getThreadHeader(nth)->t_kRet);
}


int tch_schedResumeThread(tch_thread_queue* wq,tch_threadId thread,tchStatus res,BOOL preemt){
	tch_thread_header* nth = (tch_thread_header*)thread;
	if(tch_listIsEmpty(wq)){
		return FALSE;
	}
	if(!tch_listRemove((tch_lnode_t*) wq,&nth->t_waitNode)){    // try remove given thread from wait Q
		return FALSE;                                  // if thread is not in the wait Q return
	}
	nth->t_waitQ = NULL;
	if(nth->t_to)
		tch_systimeCancelTimeout(nth);
	tch_kernelSetResult(nth,res);                                // <- check really necessary
	if(tch_schedIsPreemptable(nth) && preemt){
		nth->t_state = RUNNING;
		tch_schedReadyThread(tch_currentThread);                       // put current thread in ready queue for preemption
		getListNode(nth)->prev = getListNode(tch_currentThread); // set current thread as previous node of new thread
		tch_currentThread = nth;                                 // set new thread as current thread
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,res);    // go to switch context
		return TRUE;
	}
	tch_schedReadyThread(nth);
	return TRUE;

}

BOOL tch_schedResumeM(tch_thread_queue* wq,int cnt,tchStatus res,BOOL preemt){
	tch_thread_header* nth = NULL;
	tch_thread_header* tpreempt = NULL;
	if(tch_listIsEmpty(wq))
		return FALSE;
	while((!tch_listIsEmpty(wq)) && (cnt--)){
		tch_thread_header* nth = (tch_thread_header*) ((tch_lnode_t*) tch_listDequeue((tch_lnode_t*) wq) - 1);
		nth->t_waitQ = NULL;
		if(nth->t_to)
			tch_systimeCancelTimeout(nth);
		tch_kernelSetResult(nth,res);
		if(tch_schedIsPreemptable(nth) && preemt){
			tpreempt = nth;
		}else{
			tch_schedReadyThread(nth);
		}
	}
	if(tpreempt && preemt){
		tpreempt->t_state = RUNNING;
		tch_schedReadyThread(tch_currentThread);
		getListNode(tpreempt)->prev = getListNode(tch_currentThread);
		tch_currentThread = tpreempt;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)tpreempt,(uint32_t)getListNode(tpreempt)->prev,res);        ///< is new thread has higher priority, switch context and caller thread will get 'osOk' as a return value
	}
	return TRUE;
}

void tch_schedTryPreemption(void){
	tch_thread_header* nth = NULL;
	if(tch_listIsEmpty(&tch_readyQue) || !tch_schedIsPreemptable(tch_readyQue.thque.next))
		return;
	nth = (tch_thread_header*)tch_listDequeue((tch_lnode_t*)&tch_readyQue);
	tch_listEnqueuePriority((tch_lnode_t*) &tch_readyQue,getListNode(tch_currentThread),tch_schedReadyQRule);
	getListNode(nth)->prev = getListNode(tch_currentThread);
	tch_currentThread = nth;
	getThreadHeader(getListNode(nth)->prev)->t_state = SLEEP;
	getThreadHeader(tch_currentThread)->t_state = RUNNING;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t) nth,(uint32_t)getListNode(nth)->prev,getThreadHeader(nth)->t_kRet);   // switch context without change
}


BOOL tch_schedIsEmpty(){
	return tch_listIsEmpty(&tch_readyQue);
}

BOOL tch_schedIsPreemptable(tch_threadId nth){
	if(!nth)
		return FALSE;
	return tch_schedReadyQRule(nth,tch_currentThread) || ((tch_currentThread->t_tslot > 10) && (getThreadHeader(nth)->t_prior != Idle));
}

void tch_schedTerminateThread(tch_threadId thread,int result){
	tch_threadId jth = 0;
	tch_thread_header* cth_p = getThreadHeader(thread);
	cth_p->t_state = TERMINATED;                                                      ///< change state of thread to terminated
	while(!tch_listIsEmpty(&cth_p->t_joinQ)){
		jth = (tch_threadId)((tch_lnode_t*) tch_listDequeue(&cth_p->t_joinQ) - 1);    ///< dequeue thread(s) waiting for termination of this thread
		getThreadHeader(jth)->t_waitQ = NULL;
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,jth,tch_schedReadyQRule);
		tch_kernelSetResult(jth,result);
	}
	tch_currentThread = tch_listDequeue((tch_lnode_t*) &tch_readyQue);
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)tch_currentThread,(uint32_t)cth_p,tch_currentThread->t_kRet);
}


void tch_schedDestroyThread(tch_threadId thread,int res){
	/// manipulated exc stack to return atexit
	if(thread == tch_currentThread){
		tch_port_jmpToKernelModeThread(__tch_kernel_atexit,(uint32_t)thread,res,0);
	}else{
		getThreadHeader(thread)->t_flag |= THREAD_DEATH_BIT;
		getThreadHeader(thread)->t_reent._errno = res;
	}
}

BOOL tch_schedLivenessChk(tch_threadId thread){
	if((getThreadHeader(thread)->t_flag & THREAD_DEATH_BIT)){
		tch_port_jmpToKernelModeThread(__tch_kernel_atexit,(uint32_t)thread,getThreadHeader(thread)->t_reent._errno,0);
		getThreadHeader(thread)->t_flag &= ~THREAD_DEATH_BIT;
		return FALSE;
	}
	if((*getThreadHeader(thread)->t_chks != (uint32_t)tch_noop_destr)){  // stack overflow
		getThreadHeader(thread)->t_reent._errno = tchErrorNoMemory;
		tch_port_jmpToKernelModeThread(__tch_kernel_atexit,(uint32_t)thread,getThreadHeader(thread)->t_reent._errno,0);
		return FALSE;
	}

	return TRUE;

}



static inline void tch_schedInitKernelThread(tch_threadId init_thr){
	tch_thread_header* thr_p = (tch_thread_header*) init_thr;
	tch_port_setThreadSP((uint32_t)thr_p->t_ctx);
#ifdef MFEATURE_HFLOAT
		float _force_fctx = 0.1f;
		_force_fctx += 0.1f;
#endif

	thr_p->t_state = RUNNING;
	int result = thr_p->t_fn(thr_p->t_arg);
	tch_port_enterSv(SV_THREAD_TERMINATE,(uint32_t) thr_p,result);
}




static DECLARE_COMPARE_FN(tch_schedReadyQRule){
	return getThreadHeader(prior)->t_prior > getThreadHeader(post)->t_prior;
}



static DECLARE_COMPARE_FN(tch_schedWqRule){
	return TRUE;
}

