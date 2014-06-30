/*
 * tch_sched.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_sched.h"


/* =================  private internal function declaration   ========================== */


#ifndef MAIN_STACK_SIZE
#define MAIN_STACK_SIZE                    (1 << 10)
#endif

#ifndef IDLE_STACK_SIZE
#define IDLE_STACK_SIZE                    (1 << 8)
#endif

#define getThreadHeader(th_id)  ((tch_thread_header*) th_id)
#define getListNode(th_id)    ((tch_genericList_node_t*) th_id)



/***
 *  main thread routine
 */
extern void* main(void* arg);

/***
 *  idle thread routine
 */
static void* idle(void* arg);
/***
 *  Invoked when thread complete its task
 */
static void tch_schedThreadExit(tch_thread_id thr);


/***
 *  Scheduling policy Implementation
 */
static LIST_COMPARE_FN(tch_schedReadyQPolicy);
static LIST_COMPARE_FN(tch_schedPendQPolicy);

/***
 *  Invoked when new thread start
 */
static inline void tch_schedInitKernelThread(tch_thread_id thr)__attribute__((always_inline));

static tch_thread_id MainThread_id;
static uint32_t MAIN_STACK[MAIN_STACK_SIZE];


static tch_thread_id IdleThread_id;
static uint32_t IDLE_STACK[IDLE_STACK_SIZE];


static tch_thread_queue tch_readyQue;        ///< thread wait to become running state
static tch_thread_queue tch_pendQue;         ///< thread wait to become ready state after being suspended


static tch_thread_id     tch_currentThread;
static tch_kernel_instance*   _sys;
static tch_port_ix*     _port;
static uint64_t          tch_systimeTick;



void tch_schedInit(void* arg){
	_sys = (tch_kernel_instance*) arg;
	_port = _sys->tch_port;

	/**
	 *  Initialize ready & pend queue
	 *   - ready queue : waiting queue in which active thread is waiting to occupy cpu run time (to be executed)
	 *   - pend queue : waiting queue in which suspended thread is waiting to be re activated
	 *
	 */
	tch_genericQue_Init((tch_genericList_queue_t*)&tch_readyQue);
	tch_genericQue_Init((tch_genericList_queue_t*)&tch_pendQue);

	tch_systimeTick = 0;                        ///< main system timer tick
	_port->_enableSysTick();                    ///< enable system timer tick

	tch_thread_cfg thcfg;
	thcfg._t_routine = main;
	thcfg._t_stack = MAIN_STACK;
	thcfg.t_stackSize = MAIN_STACK_SIZE;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";
	MainThread_id = Thread->create((tch*)_sys,&thcfg,_sys);

	thcfg._t_routine = idle;
	thcfg._t_stack = IDLE_STACK;
	thcfg.t_stackSize = IDLE_STACK_SIZE;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	IdleThread_id = Thread->create((tch*)_sys,&thcfg,_sys);

	tch_currentThread = IdleThread_id;
	tch_schedInitKernelThread(IdleThread_id);
	while(true){
		__WFI();
	}

}





/** Note : should not called any other program except kernel mode program
 *  start thread based on its priority (thread can be started in preempted way)
 */
void tch_schedStartThread(tch_thread_id nth){
	tch_thread_header* thr_p = nth;
	if(!_port)
		tch_error_handler(false,osErrorOS);             ///< ensure portibility
	if(tch_schedIsPreemtable(nth)){
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&tch_readyQue,getListNode(tch_currentThread),tch_schedReadyQPolicy);			///< put current thread in ready queue
		getListNode(nth)->prev = tch_currentThread; 	                                                            ///< set new thread as current thread
		getThreadHeader(tch_currentThread)->t_state = READY;
		tch_currentThread = nth;
		_port->_jmpToKernelModeThread(_port->_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev);
	}else{
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&tch_readyQue,getListNode(nth),tch_schedReadyQPolicy);
	}
}


/**
 * put new thread into ready queue
 */
void tch_schedScheduleToReady(tch_thread_id th){
	tch_thread_header* thr_p = th;
	if(!_port)
		tch_error_handler(false,osErrorOS);
	tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&tch_readyQue,getListNode(thr_p),tch_schedReadyQPolicy);
}

/** Note : should not called any other program except kernel mode program
 *  make current thread sleep for specified amount of time (yield cpu time)
 */
BOOL tch_schedScheduleToSuspend(uint32_t timeout){
	tch_thread_id nth = 0;
	if(!_port)
		tch_error_handler(false,osErrorOS);
	getThreadHeader(tch_currentThread)->t_to = tch_systimeTick + timeout;
	tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&tch_pendQue,getListNode(tch_currentThread),tch_schedPendQPolicy);
	nth = tch_genericQue_dequeue((tch_genericList_queue_t*)&tch_readyQue);
	getListNode(nth)->prev = tch_currentThread;
	tch_currentThread = nth;
	_port->_jmpToKernelModeThread(_port->_switchContext,(uint32_t)nth,(uint32_t)getListNode(nth)->prev);
	return true;
}

BOOL tch_schedJoin(tch_thread_id thr_id){

}


/***
 *  System Timer Handler
 */
void tch_kernelSysTick(void){
	tch_systimeTick++;
	tch_thread_header* nth = NULL;
	while(getThreadHeader(tch_pendQue.thque.entry)->t_to <= tch_systimeTick){                      ///< Check timeout value
		nth = (tch_thread_header*) tch_genericQue_dequeue((tch_genericList_queue_t*)&tch_pendQue);
		nth->t_state = READY;
		tch_schedScheduleToReady(nth);
	}
	if(tch_readyQue.thque.entry && tch_schedIsPreemtable(tch_readyQue.thque.entry)){
		tch_thread_header* nth = (tch_thread_header*)tch_genericQue_dequeue((tch_genericList_queue_t*)&tch_readyQue);
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*) &tch_readyQue,tch_currentThread,tch_schedReadyQPolicy);
		getListNode(nth)->prev = tch_currentThread;
		tch_currentThread = nth;
		getThreadHeader(getListNode(nth)->prev)->t_state = SLEEP;
		getThreadHeader(tch_currentThread)->t_state = RUNNING;
		_port->_jmpToKernelModeThread(_port->_switchContext,(uint32_t) nth,(uint32_t)getListNode(nth)->prev);
	}
}

tch_thread_id tch_schedGetRunningThread(){
	return tch_currentThread;
}



BOOL tch_schedIsPreemtable(tch_thread_id nth){
	return tch_schedReadyQPolicy(tch_currentThread,nth);
}

void tch_schedTerminate(tch_thread_id thread){
	tch_thread_id jth = 0;
	tch_thread_header* cth_p = getThreadHeader(thread);
	if(!_port)
		tch_error_handler(false,osErrorOS);
	cth_p->t_state = TERMINATED;                          ///< change state of thread to terminated
	while(cth_p->t_joinQ.entry){
		jth = tch_genericQue_dequeue(&cth_p->t_joinQ);    ///< dequeue thread(s) waiting for termination of this thread
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&tch_readyQue,jth,tch_schedReadyQPolicy);
	}
	///< optional on terminated hook here
}


static inline void tch_schedInitKernelThread(tch_thread_id init_thr){
	tch_thread_header* thr_p = (tch_thread_header*) init_thr;
	tch_kernel_instance* kins = (tch_kernel_instance*)thr_p->t_sys;
	_port_setThreadSP((uint32_t)thr_p->t_ctx);
	#ifdef FEATURE_HFLOAT
		float _force_fctx = 0.1f;
		_force_fctx += 0.1f;
	#endif

		thr_p->t_state = RUNNING;
		tch_msg* msg = thr_p->t_fn(thr_p->t_arg);
		kins->tch_port->_enterSvFromUsr(SV_THREAD_TERMINATE,(uint32_t) thr_p,0);
}

LIST_COMPARE_FN(tch_schedReadyQPolicy){
	return getThreadHeader(li0)->t_prior < getThreadHeader(li1)->t_prior;
}

LIST_COMPARE_FN(tch_schedPendQPolicy){
	return getThreadHeader(li0)->t_to > getThreadHeader(li1)->t_to;
}

void* idle(void* arg){
	/**
	 * idle init
	 * - idle indicator init
	 */
	tch_kernel_instance* _sys = (tch_kernel_instance*) arg;
	_sys->tch_api.Thread->start(MainThread_id);
	while(true){
		__DMB();
		__ISB();
		__WFI();
		__DMB();
		__ISB();
	}
	return NULL;
}
