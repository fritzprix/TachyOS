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
	uint32_t val = 0;
	float hv = 0.1f;
	osStatus sleeprst = osErrorOS;



	mtxid = Mtx->create(&tMtx);

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
	osStatus result = osOK;
	val++;
	hv += 0.2f;
	result = tch_api->Mtx->lock(mtxid,1);
	tch_api->Thread->sleep(17);
	tch_api->Mtx->unlock(mtxid);
	sleeprst = Thread->sleep(1000);
	tch_api->Mtx->destroy(mtxid);
	while(1){
		tch_api->Thread->sleep(10);
	}
	return 0;
}


DECLARE_THREADROUTINE(childThread1_routine){
	tch* tch_api = (tch*) arg;
	osStatus result = tch_api->Mtx->lock(mtxid,0);
	return 0;
}

DECLARE_THREADROUTINE(childThread2_routine){
	tch* tch_api = (tch*) arg;
	osStatus result = tch_api->Mtx->lock(mtxid,0);
	while(1){
		tch_api->Thread->sleep(10);
	}
	return 0;
}

