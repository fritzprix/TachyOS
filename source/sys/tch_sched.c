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
#include "tch_port.h"
#include "tch_list.h"
#include "tch_sched.h"
#include "tch_halcfg.h"


/* =================  private internal function declaration   ========================== */



#define getThreadHeader(th_id)  ((tch_thread_header*) th_id)
#define getListNode(th_id)    ((tch_lnode_t*) th_id)



/***
 *  main thread routine
 */
extern int main(void* arg);

/***
 *  idle thread routine
 */
static int idle(void* arg);


/***
 *  Scheduling policy Implementation
 */
static LIST_CMP_FN(tch_schedReadyQPolicy);
static LIST_CMP_FN(tch_schedPendQPolicy);

/***
 *  Invoked when new thread start
 */
static inline void tch_schedInitKernelThread(tch_thread_id thr)__attribute__((always_inline));

static tch_thread_id MainThread_id;
static tch_thread_id IdleThread_id;



static tch_thread_queue tch_readyQue;        ///< thread wait to become running state
static tch_thread_queue tch_pendQue;         ///< thread wait to become ready state after being suspended


static tch_thread_id          tch_currentThread;
static tch_kernel_instance*   _sys;
static uint64_t               tch_systimeTick;



void tch_schedInit(void* arg){
	_sys = (tch_kernel_instance*) arg;

	/**
	 *  Initialize ready & pend queue
	 *   - ready queue : waiting queue in which active thread is waiting to occupy cpu run time (to be executed)
	 *   - pend queue : waiting queue in which suspended thread is waiting to be re activated
	 *
	 */
	tch_listInit((tch_lnode_t*)&tch_readyQue);
	tch_listInit((tch_lnode_t*)&tch_pendQue);

	tch_systimeTick = 0;                        ///< main system timer tick
	tch_port_enableSysTick();                    ///< enable system timer tick

	tch_thread_cfg thcfg;
	thcfg._t_routine = main;
	thcfg._t_stack = &Main_Stack_Limit;
	thcfg.t_stackSize = (uint32_t) &Main_Stack_Top - (uint32_t) &Main_Stack_Limit;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";
	MainThread_id = Thread->create(&thcfg,_sys);

	thcfg._t_routine = idle;
	thcfg._t_stack = &Idle_Stack_Limit;
	thcfg.t_stackSize = (uint32_t)&Idle_Stack_Top - (uint32_t)&Idle_Stack_Limit;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	IdleThread_id = Thread->create(&thcfg,_sys);

	tch_currentThread = IdleThread_id;
	tch_schedInitKernelThread(IdleThread_id);


	while(TRUE){     // Unreachable Code. Ensure to protect from executing when undetected schedulr failure happens
		__WFI();
	}
}


/** Note : should not called any other program except kernel mode program
 *  @brief start thread based on its priority (thread can be started in preempted way)
 */
void tch_schedStartThread(tch_thread_id nth){
	tch_thread_header* thr_p = nth;
	if(tch_schedIsPreemptable(nth)){
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(tch_currentThread),tch_schedReadyQPolicy);			///< put current thread in ready queue
		getListNode(nth)->prev = tch_currentThread; 	                                                            ///< set new thread as current thread
		tch_currentThread = nth;
		getThreadHeader(tch_currentThread)->t_state = RUNNING;
		getThreadHeader(getListNode(tch_currentThread)->prev)->t_state = READY;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,osOK);
	}else{
		getThreadHeader(nth)->t_state = READY;
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(nth),tch_schedReadyQPolicy);
	}
}


/**
 * put new thread into ready queue
 */
void tch_schedReady(tch_thread_id th){
	tch_thread_header* thr_p = th;
	getThreadHeader(th)->t_state = READY;
	tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,getListNode(thr_p),tch_schedReadyQPolicy);
}

/** Note : should not called any other program except kernel mode program
 *  make current thread sleep for specified amount of time (yield cpu time)
 */
void tch_schedSleep(uint32_t timeout,tch_thread_state nextState){
	tch_thread_id nth = 0;
	getThreadHeader(tch_currentThread)->t_to = tch_systimeTick + timeout;
	tch_listEnqueuePriority((tch_lnode_t*)&tch_pendQue,getListNode(tch_currentThread),tch_schedPendQPolicy);
	nth = tch_listDequeue((tch_lnode_t*)&tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,osErrorOS);
	getListNode(nth)->prev = tch_currentThread;
	tch_currentThread = nth;
	getThreadHeader(tch_currentThread)->t_state = RUNNING;
	getThreadHeader(getListNode(tch_currentThread)->prev)->t_state = nextState;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,osEventTimeout);
}


void tch_schedSuspend(tch_thread_queue* wq,uint32_t timeout){
	tch_thread_id nth = 0;
	if(timeout != osWaitForever){
		getThreadHeader(tch_currentThread)->t_to = tch_systimeTick + timeout;
		tch_listEnqueuePriority((tch_lnode_t*) &tch_pendQue,getListNode(tch_currentThread),tch_schedPendQPolicy);

	}
	tch_listEnqueuePriority((tch_lnode_t*) wq,&getThreadHeader(tch_currentThread)->t_waitNode,tch_schedReadyQPolicy);
	nth = tch_listDequeue((tch_lnode_t*) &tch_readyQue);
	if(!nth)
		tch_kernel_errorHandler(FALSE,osErrorOS);
	getListNode(nth)->prev = tch_currentThread;
	getThreadHeader(nth)->t_state = RUNNING;
	getThreadHeader(tch_currentThread)->t_state = WAIT;
	getThreadHeader(tch_currentThread)->t_waitQ = (tch_lnode_t*)wq;
	tch_currentThread = nth;
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,osErrorTimeoutResource);
}


tch_thread_header* tch_schedResume(tch_thread_queue* wq){
	if(tch_listIsEmpty(wq))
		return NULL;
	tch_thread_header* nth = (tch_thread_header*) ((tch_lnode_t*) tch_listDequeue((tch_lnode_t*) wq) - 1);
	nth->t_waitQ = NULL;
	if(tch_schedIsPreemptable(nth)){
		nth->t_state = RUNNING;
		getThreadHeader(tch_currentThread)->t_state = READY;
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,tch_currentThread,tch_schedReadyQPolicy);
		getListNode(nth)->prev = tch_currentThread;
		tch_currentThread = nth;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev,osOK);        ///< is new thread has higher priority, switch context and caller thread will get 'osOk' as a return value
		tch_currentThread = nth;
	}else{
		nth->t_state = READY;
		nth->t_ctx->kRetv = osOK;
		tch_listEnqueuePriority((tch_lnode_t*) &tch_readyQue,(tch_lnode_t*)nth,tch_schedReadyQPolicy);
	}
	return nth;
}


uint64_t tch_kernelCurrentSystick(){
	return tch_systimeTick;
}


void tch_schedResumeAll(tch_thread_queue* wq){
	tch_thread_header* nth = NULL;
	tch_thread_header* tpreempt = NULL;
	if(tch_listIsEmpty(wq))
		return;
	while(!tch_listIsEmpty(wq)){
		tch_thread_header* nth = (tch_thread_header*) ((tch_lnode_t*) tch_listDequeue((tch_lnode_t*) wq) - 1);
		if(tch_schedIsPreemptable(nth)){
			tpreempt = nth;
		}else{
			nth->t_waitQ = NULL;
			nth->t_state = READY;
			nth->t_ctx->kRetv = osOK;
			tch_listEnqueuePriority((tch_lnode_t*) &tch_readyQue,(tch_lnode_t*)nth,tch_schedReadyQPolicy);
		}
	}
	if(tpreempt){
		tpreempt->t_state = RUNNING;
		getThreadHeader(tch_currentThread)->t_state = READY;
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,tch_currentThread,tch_schedReadyQPolicy);
		getListNode(tpreempt)->prev = tch_currentThread;
		tch_currentThread = tpreempt;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)tpreempt,(uint32_t)getListNode(tpreempt)->prev,osOK);        ///< is new thread has higher priority, switch context and caller thread will get 'osOk' as a return value
		tch_currentThread = tpreempt;
	}

}



tchStatus tch_schedCancelTimeout(tch_thread_id thread){
	tch_genericQue_remove((tch_lnode_t*)&tch_pendQue,(tch_lnode_t*)thread);
	return osOK;
}


/***
 *  System Timer Handler
 */
void tch_kernelSysTick(void){
	tch_systimeTick++;
	tch_thread_header* nth = NULL;
	while((!tch_listIsEmpty(&tch_pendQue)) && (getThreadHeader(tch_pendQue.thque.next)->t_to <= tch_systimeTick)){                      ///< Check timeout value
		nth = (tch_thread_header*) tch_listDequeue((tch_lnode_t*)&tch_pendQue);
		nth->t_state = READY;
		tch_schedReady(nth);
		if(nth->t_waitQ){
			tch_listRemove(nth->t_waitQ,&getThreadHeader(nth)->t_waitNode);        // cancel wait to lock mutex
			nth->t_waitQ = NULL;                                                   // set reference to wait queue to null
		}
	}
	if((!tch_listIsEmpty(&tch_readyQue)) && tch_schedIsPreemptable(tch_readyQue.thque.next)){
		tch_thread_header* nth = (tch_thread_header*)tch_listDequeue((tch_lnode_t*)&tch_readyQue);
		tch_listEnqueuePriority((tch_lnode_t*) &tch_readyQue,tch_currentThread,tch_schedReadyQPolicy);
		getListNode(nth)->prev = tch_currentThread;
		tch_currentThread = nth;
		getThreadHeader(getListNode(nth)->prev)->t_state = SLEEP;
		getThreadHeader(tch_currentThread)->t_state = RUNNING;
		tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t) nth,(uint32_t)getListNode(nth)->prev,osOK);
	}
}

tch_thread_id tch_schedGetRunningThread(){
	return tch_currentThread;
}



BOOL tch_schedIsPreemptable(tch_thread_id nth){
	if(!nth)
		return FALSE;
	return tch_schedReadyQPolicy(nth,tch_currentThread);
}

void tch_schedTerminate(tch_thread_id thread,int result){
	tch_thread_id jth = 0;
	tch_thread_header* cth_p = getThreadHeader(thread);
	cth_p->t_state = TERMINATED;                          ///< change state of thread to terminated
	while(!tch_listIsEmpty(&cth_p->t_joinQ)){
		jth = (tch_thread_id)((tch_lnode_t*) tch_listDequeue(&cth_p->t_joinQ) - 1);    ///< dequeue thread(s) waiting for termination of this thread
		tch_listEnqueuePriority((tch_lnode_t*)&tch_readyQue,jth,tch_schedReadyQPolicy);
		getThreadHeader(jth)->t_ctx->kRetv = result;
	}
	tch_currentThread = tch_listDequeue((tch_lnode_t*) &tch_readyQue);
	tch_port_jmpToKernelModeThread(tch_port_switchContext,(uint32_t)tch_currentThread,(uint32_t)cth_p,osOK);
}


static inline void tch_schedInitKernelThread(tch_thread_id init_thr){
	tch_thread_header* thr_p = (tch_thread_header*) init_thr;
	tch_port_setThreadSP((uint32_t)thr_p->t_ctx);
#ifdef MFEATURE_HFLOAT
		float _force_fctx = 0.1f;
		_force_fctx += 0.1f;
#endif

		thr_p->t_state = RUNNING;
		int result = thr_p->t_fn(thr_p->t_arg);
		tch_port_enterSvFromUsr(SV_THREAD_TERMINATE,(uint32_t) thr_p,result);
}

LIST_CMP_FN(tch_schedReadyQPolicy){
	return getThreadHeader(prior)->t_prior > getThreadHeader(post)->t_prior;
}

LIST_CMP_FN(tch_schedPendQPolicy){
	return getThreadHeader(prior)->t_to < getThreadHeader(post)->t_to;
}

int idle(void* arg){
	/**
	 * idle init
	 * - idle indicator init
	 */
	tch_kernel_instance* _sys = (tch_kernel_instance*) arg;
	_sys->tch_api.Thread->start(MainThread_id);
	while(TRUE){
		__DMB();
		__ISB();
		__WFI();
		__DMB();
		__ISB();
	}
	return 0;
}
