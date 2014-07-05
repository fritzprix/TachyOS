/*
 * main.c
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"
#include "tch.h"

static tch_mtx tMtx;
static tch_mtx_id mtxid;


static DECLARE_THREADROUTINE(childThread1_routine);
static DECLARE_THREADSTACK(childThread1_stack,1 << 10);
static tch_thread_id childThread1;

static DECLARE_THREADROUTINE(childThread2_routine);
static DECLARE_THREADSTACK(childThread2_stack,1 << 10);
static tch_thread_id childThread2;




void* main(void* arg) {
	tch* tch_api = (tch*)arg;
	tch_thread_ix* Thread = (tch_thread_ix*) tch_api->Thread;
	tch_mtx_ix* Mtx = (tch_mtx_ix*) tch_api->Mtx;
	tch_signal_ix* Sig = (tch_signal_ix*) tch_api->Sig;


	tch_thread_cfg tcfg;
	tcfg._t_name = "child1";
	tcfg._t_routine = childThread1_routine;
	tcfg._t_stack = childThread1_stack;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize  = 1 << 8;
	childThread1 = Thread->create(&tcfg,arg);
	Thread->start(childThread1);

	tcfg._t_name = "child2";
	tcfg._t_routine = childThread2_routine;
	tcfg._t_stack = childThread2_stack;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 1 << 8;
	childThread2 = Thread->create(&tcfg,arg);
	Thread->start(childThread2);

	Thread->sleep(1000);
	Sig->set(childThread1,1);
	Sig->set(childThread2,1);
	while(1){
		tch_api->Thread->sleep(10);
	}
	return 0;
}


DECLARE_THREADROUTINE(childThread1_routine){
	tch* api = (tch*) arg;
	osStatus result = api->Sig->wait(1,osWaitForever);
	return 0;
}

DECLARE_THREADROUTINE(childThread2_routine){
	tch* api = (tch*) arg;
	osStatus result = api->Sig->wait((1 << 5) | (1 << 3),3000);
	return 0;
}

