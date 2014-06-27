/*
 * tch_sched.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_sched.h"


/* =================  private internal function declaration   ========================== */




/***
 *  Invoked when thread complete its task
 */
static void tch_schedTerminate(tch_thread_id thread);
static LIST_COMPARE_FN(tch_schedPolicy);


static tch_thread_queue tch_readyQue;        ///< thread wait to become running state
static tch_thread_queue tch_pendQue;         ///< thread wait to become ready state after being suspended


static tch_thread_id     tch_currentThread;
static tch_prototype*   _sys;
static tch_port_ix*     _port;



void tch_schedInit(void* arg){
	_sys = (tch_prototype*) arg;
	_port = _sys->tch_port;

	tch_genericQue_Init((tch_genericList_queue_t*)&tch_readyQue);
	tch_genericQue_Init((tch_genericList_queue_t*)&tch_pendQue);
}



/**
 * this function is only invoked by kernel
 * - kernel lock should be guaranteed
 *
 */
void tch_schedStartThread(tch_thread_id nth){
	tch_thread_header* thr_p = nth;
	if(!_port)
		tch_error_handler(false,osErrorOS);             ///< ensure portibility
	_port->_kernel_lock();
	tch_currentThread = nth;                            ///< set new thread as current thread
	thr_p->t_state = RUNNING;
	_port->_setThreadSP((uint32_t)thr_p->t_ctx);        ///< set stack pointer
	_port->_kernel_unlock();
	thr_p->t_fn(_sys);
	tch_schedTerminate(nth);
}

void tch_schedScheduleThread(tch_thread_id th){
	tch_thread_header* thr_p = th;
	if(!_port)
		tch_error_handler(false,osErrorOS);
	tch_genericQue_enqueueWithCompare(&tch_readyQue,thr_p,tch_schedPolicy);
}


BOOL tch_schedSuspend(uint32_t timeout){

}

BOOL tch_schedJoin(tch_thread_id thr_id){

}


void tch_schedTerminate(tch_thread_id thread){

}
