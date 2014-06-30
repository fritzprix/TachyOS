/*
 * tch_thread.c
 *
 *  Created on: 2014. 6. 17.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_sched.h"



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
static tch_thread_id tch_threadCreate(tch* _sys,tch_thread_cfg* cfg,void* arg);
static void tch_threadStart(tch_thread_id thread);
static osStatus tch_threadTerminate(tch_thread_id thread);
static tch_thread_id tch_threadSelf();
static osStatus tch_threadSleep(uint32_t millisec);
static osStatus tch_threadJoin(tch_thread_id thread);
static void tch_threadSetPriority(tch* _sys,tch_thread_prior nprior);
static tch_thread_prior tch_threadGetPriorty(tch* _sys);


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

tch_thread_id tch_threadCreate(tch* _sys,tch_thread_cfg* cfg,void* arg){
	uint32_t* sptop = (uint32_t*)cfg->_t_stack + cfg->t_stackSize;             /// peek stack top pointer
	tch_port_ix* port = ((tch_kernel_instance*) _sys)->tch_port;

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
	thread_p->t_ctx = port->_makeInitialContext(thread_p,__tch_thread_entry);                // manipulate initial context of thread
	thread_p->t_sys = _sys;

	thread_p->t_to = 0;
	thread_p->t_id = (uint32_t) thread_p;
	thread_p->t_lckCnt = 0;
	thread_p->t_listNode.next = thread_p->t_listNode.prev = NULL;
	tch_genericQue_Init(&thread_p->t_joinQ);

	return (tch_thread_id) thread_p;

}

void tch_threadStart(tch_thread_id thread){
	tch_port_ix* tch_port = ((tch_kernel_instance*) getThreadHeader(thread)->t_sys)->tch_port;
	if(__get_IPSR()){
		// in isr mode, directly starting thread  is prohibited
		tch_schedScheduleToReady(thread);
		//    tch_port->_enterSvFromIsr(SV_THREAD_START,(uint32_t)thread,0);
	}else{
		// thread mode
		tch_port->_enterSvFromUsr(SV_THREAD_START,(uint32_t)thread,0);
	}
}

/***
 *  I Think force thread to be terminated directly from other user thread is not good in design perspective
 *  if there is any good reason comments below
 */
osStatus tch_threadTerminate(tch_thread_id thread){
	// not implemented
	return osErrorOS;
}

tch_thread_id tch_threadSelf(){
	return tch_schedGetRunningThread();
}

osStatus tch_threadSleep(uint32_t millisec){
	tch_port_ix* tch_port = ((tch_kernel_instance*) getThreadHeader(tch_schedGetRunningThread())->t_sys)->tch_port;
	if(__get_IPSR()){
		tch_error_handler(false,osErrorOS);
		return osErrorOS;
	}else{
		tch_port->_enterSvFromUsr(SV_THREAD_SLEEP,millisec,0);
		return osOK;
	}
}

osStatus tch_threadJoin(tch_thread_id thread){

}

void tch_threadSetPriority(tch* _sys,tch_thread_prior nprior){

}

tch_thread_prior tch_threadGetPriorty(tch* _sys){

}

void __tch_thread_entry(void){
	tch_thread_header* thr_p = (tch_thread_header*) tch_schedGetRunningThread();
	tch_kernel_instance* kins = (tch_kernel_instance*)thr_p->t_sys;

#ifdef FEATURE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif

	thr_p->t_state = RUNNING;
	tch_msg* msg = thr_p->t_fn(thr_p->t_arg);
	kins->tch_port->_enterSvFromUsr(SV_THREAD_TERMINATE,(uint32_t) thr_p,0);
}

