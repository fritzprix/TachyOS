/*
 * tch_thread.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 17.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_sched.h"



#define getThreadHeader(th_id)  ((tch_thread_header*) th_id)


/**
 *  public accessible function
 */
/***
 *  create new thread
 *   - inflate thread header structure onto the very top of thread stack area
 *   - initialize thread header from thread configuration type
 *   - inflate thread context for start (R4~LR)
 *     -> LR point Entry Routine of Thread
 *
 */
static tch_thread_id tch_threadCreate(tch_thread_cfg* cfg,void* arg);
static tchStatus tch_threadStart(tch_thread_id thread);
static tchStatus tch_threadTerminate(tch_thread_id thread);
static tch_thread_id tch_threadSelf();
static tchStatus tch_threadSleep(uint32_t millisec);
static tchStatus tch_threadJoin(tch_thread_id thread,uint32_t timeout);
static void tch_threadSetPriority(tch_thread_prior nprior);
static tch_thread_prior tch_threadGetPriorty();


static void __tch_thread_entry(void);

static tch_thread_ix tch_threadix = {
		tch_threadCreate,
		tch_threadStart,
		tch_threadTerminate,
		tch_threadSelf,
		tch_threadSleep,
		tch_threadJoin,
		tch_threadSetPriority,
		tch_threadGetPriorty
};


const tch_thread_ix* Thread = &tch_threadix;

tch_thread_id tch_threadCreate(tch_thread_cfg* cfg,void* arg){
	uint8_t* sptop = (uint8_t*)cfg->_t_stack + cfg->t_stackSize;             /// peek stack top pointer

	/**
	 * thread initialize from configuration type
	 */
	tch_thread_header* thread_p = ((tch_thread_header*) sptop - 1);
	thread_p->t_arg = arg;
	thread_p->t_fn = cfg->_t_routine;
	thread_p->t_name = cfg->_t_name;
	thread_p->t_prior = thread_p->t_svd_prior = cfg->t_proior;



	                                                                             /**
	                                                                             *  thread context will be saved on 't_ctx'
	                                                                             *  initial sp is located in 2 context table offset below thread pointer
	                                                                             *  and context is manipulted to direct thread to thread start point
	                                                                             */
	thread_p->t_ctx = tch_port_makeInitialContext(thread_p,__tch_thread_entry);                // manipulate initial context of thread

	thread_p->t_to = 0;
	thread_p->t_lckCnt = 0;
	thread_p->t_schedNode.next = thread_p->t_schedNode.prev = NULL;
	thread_p->t_waitNode.next = thread_p->t_waitNode.prev = NULL;
	thread_p->t_waitQ = NULL;
	thread_p->t_sig.match_target = thread_p->t_sig.signal = 0;
	tch_genericQue_Init(&thread_p->t_sig.sig_wq);
	tch_genericQue_Init(&thread_p->t_joinQ);
	thread_p->t_chks = (uint32_t)thread_p->t_arg + (uint32_t)thread_p->t_fn;

	return (tch_thread_id) thread_p;

}

tchStatus tch_threadStart(tch_thread_id thread){
	if(tch_port_isISR()){          ///< check current execution mode (Thread or Handler)
		tch_schedReady(thread);    ///< if handler mode call, put current thread in ready queue
		                                     ///< optionally check preemption required or not
		return osOK;
	}else{
		return tch_port_enterSvFromUsr(SV_THREAD_START,(uint32_t)thread,0);
	}
}

/***
 *  I Think force thread to be terminated directly from other user thread is not good in design perspective
 *  the only use-case of these kind of functions is when thread has gone wrong.
 *  but for such a case, those kind of function(termiating thread) should be provided as kernel feature
 *  not as user program space function. just my thought
 *
 *  if there is any new idea, comment below...
 */
tchStatus tch_threadTerminate(tch_thread_id thread){
	// not implemented
	return osErrorOS;
}

/***
 *
 */
tch_thread_id tch_threadSelf(){
	return tch_schedGetRunningThread();
}

tchStatus tch_threadSleep(uint32_t millisec){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		return tch_port_enterSvFromUsr(SV_THREAD_SLEEP,millisec,0);
	}
}

tchStatus tch_threadJoin(tch_thread_id thread,uint32_t timeout){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}else{
		if(getThreadHeader(thread)->t_state == TERMINATED){
			return osErrorParameter;
		}
		return tch_port_enterSvFromUsr(SV_THREAD_JOIN,(uint32_t) thread,timeout);
	}
}

void tch_threadSetPriority(tch_thread_prior nprior){
	getThreadHeader(tch_schedGetRunningThread())->t_prior = nprior;
}

tch_thread_prior tch_threadGetPriorty(){
	return getThreadHeader(tch_schedGetRunningThread())->t_prior;
}


BOOL tch_kernelThreadIntegrityCheck(tch_thread_id thrtochk){
	tch_thread_header* th_p = (tch_thread_header*) thrtochk;
	return th_p->t_chks == ((uint32_t)th_p->t_arg + (uint32_t)th_p->t_fn)? TRUE:FALSE;
}



void __tch_thread_entry(void){
	tch_thread_header* thr_p = (tch_thread_header*) tch_schedGetRunningThread();

#ifdef MFEAUTRE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif

	thr_p->t_state = RUNNING;
	int result = thr_p->t_fn(thr_p->t_arg);
	tch_port_enterSvFromUsr(SV_THREAD_TERMINATE,(uint32_t) thr_p,result);
}

