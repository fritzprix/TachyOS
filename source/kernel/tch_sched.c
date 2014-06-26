/*
 * tch_sched.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"

static tch_thread_queue tch_readyQue;        ///< thread queue for ready
static tch_thread_queue tch_systimeQue;      ///< thread queue for sleep
static tch_thread_id    tch_currentThread;
static tch_prototype*   _sys;
static tch_port_ix*     _port;

void tch_kernelSchedStart(void* arg){
	_sys = (tch_prototype*) arg;
	_port = _sys->tch_port;

	tch_genericQue_Init((tch_genericList_queue_t*)&tch_readyQue);
	tch_genericQue_Init((tch_genericList_queue_t*)&tch_systimeQue);
}


void tch_kernelSysTick(void){

}


/**
 * this function is only invoked by kernel
 * - kernel lock should be guaranteed
 *
 */
void tch_schedStart(tch_thread_id nth){
	tch_thread_header* thr_p = nth;
	if(!_port)
		tch_error_handler(false,osErrorOS);   ///< ensure portibility
	_port->_kernel_lock();
	tch_currentThread = nth;                  ///< set new thread as current thread
	thr_p->t_state = RUNNING;
	_port->_setThreadSP((uint32_t)thr_p->t_ctx);        ///< set stack pointer
	_port->_kernel_unlock();
	thr_p->t_fn(_sys);
	tch_schedTerminate(nth);
}

void tch_schedReady(tch_thread_id th){
	tch_thread_header* thr_p = th;
	if(!_port)
		tch_error_handler(false,osErrorOS);
}

void tch_schedSleep(uint32_t timeout){

}

int tch_schedWait(tch_genericList_queue_t* wq,uint32_t timeout){

}

int tch_schedWake(tch_genericList_queue_t* wq){

}

void tch_schedTerminate(tch_thread_id thread){

}
