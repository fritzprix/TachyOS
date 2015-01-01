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
#include "tch_thread.h"
#include "tch_mem.h"
#include "tch_syscfg.h"
#include <sys/reent.h>


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
static tch_threadId tch_threadCreate(tch_threadCfg* cfg,void* arg);
static tchStatus tch_threadStart(tch_threadId thread);
static tchStatus tch_threadTerminate(tch_threadId thread,tchStatus err);
static tch_threadId tch_threadSelf();
static tchStatus tch_threadSleep();
static tchStatus tch_threadYield(uint32_t millisec);
static tchStatus tch_threadJoin(tch_threadId thread,uint32_t timeout);
static void tch_threadSetPriority(tch_threadId id,tch_thread_prior nprior);
static tch_thread_prior tch_threadGetPriorty(tch_threadId id);
static void* tch_threadGetArg();
static BOOL tch_threadIsRoot();

static void __tch_thread_entry(tch_thread_header* thr_p,tchStatus status) __attribute__((naked));


__attribute__((section(".data"))) static tch_thread_ix tch_threadix = {
		tch_threadCreate,
		tch_threadStart,
		tch_threadTerminate,
		tch_threadSelf,
		tch_threadYield,
		tch_threadSleep,
		tch_threadJoin,
		tch_threadSetPriority,
		tch_threadGetPriorty,
		tch_threadGetArg,
		tch_threadIsRoot
};


const tch_thread_ix* Thread = &tch_threadix;
tch_thread_queue tch_procList;



tch_threadId tch_threadCreate(tch_threadCfg* cfg,void* arg){
	uint8_t* th_mem = NULL;
	uint8_t* sptop = NULL;
	uint16_t allocsz = 0;
	if(cfg->t_stackSize < TCH_CFG_THREAD_STACK_MIN_SIZE)
		cfg->t_stackSize = TCH_CFG_THREAD_STACK_MIN_SIZE;

	if(tch_currentThread == ROOT_THREAD){
		allocsz = cfg->t_stackSize + TCH_CFG_PROC_HEAP_SIZE + sizeof(tch_thread_header)/* + sizeof(struct _reent)*/;
		th_mem = kMem->alloc(allocsz);
	}else{
		allocsz = cfg->t_stackSize + sizeof(tch_thread_header);
		th_mem = uMem->alloc(allocsz);
	}

	sptop = th_mem + allocsz;
	/**
	 * thread initialize from configuration type
	 */
	tch_thread_header* thread_p = ((tch_thread_header*) sptop - 1);          /// offset thread header size
	thread_p->t_arg = arg;
	thread_p->t_fn = cfg->_t_routine;
	thread_p->t_name = cfg->_t_name;
	thread_p->t_prior = cfg->t_proior;
	tch_listInit(&thread_p->t_ualc);
	tch_listInit(&thread_p->t_shalc);
	                                                                             /**
	                                                                             *  thread context will be saved on 't_ctx'
	                                                                             *  initial sp is located in 2 context table offset below thread pointer
	                                                                             *  and context is manipulted to direct thread to thread start point
	                                                                             */
	thread_p->t_ctx = tch_port_makeInitialContext(thread_p,__tch_thread_entry);                // manipulate initial context of thread

	thread_p->t_to = 0;
	thread_p->t_state = PENDED;
	thread_p->t_lckCnt = 0;
	thread_p->t_schedNode.next = thread_p->t_schedNode.prev = NULL;
	thread_p->t_waitNode.next = thread_p->t_waitNode.prev = NULL;
	thread_p->t_waitQ = NULL;
	tch_listInit(&thread_p->t_joinQ);
	tch_listInit(&thread_p->t_childNode);
	thread_p->t_chks = (uint32_t*)th_mem;                                                    // keep allocated mem pointer to release it when this thread is destroyed
	*thread_p->t_chks = (uint32_t) tch_noop_destr;                                           // thread has no-op destructor
	thread_p->t_flag = 0;

	_REENT_INIT_PTR(&thread_p->t_reent);
	if(tch_currentThread != ROOT_THREAD){                   // if new thread is child thread
		thread_p->t_refNode.root = tch_currentThread;
		thread_p->t_mem = tch_currentThread->t_mem;         // share parent thread heap
		tch_listPutFirst(tch_currentThread->t_refNode.childs,&thread_p->t_childNode);
	}else{                                                  // if new thread is root thread
		thread_p->t_refNode.childs = kMem->alloc(sizeof(tch_lnode_t));
		tch_listInit(thread_p->t_refNode.childs);
		thread_p->t_mem = tch_memCreate(th_mem + sizeof(uint32_t*),TCH_CFG_PROC_HEAP_SIZE);
		thread_p->t_flag |= THREAD_ROOT_BIT;                   // mark as root thread
		tch_listPutFirst((tch_lnode_t*)&tch_procList,(tch_lnode_t*)&thread_p->t_childNode);
	}

	return (tch_threadId) thread_p;

}

static tchStatus tch_threadStart(tch_threadId thread){
	if(tch_port_isISR()){          // check current execution mode (Thread or Handler)
		tch_schedReadyThread(thread);    // if handler mode call, put current thread in ready queue
		                           // optionally check preemption required or not
		return tchOK;
	}else{
		return tch_port_enterSv(SV_THREAD_START,(uint32_t)thread,0);
	}
}

/*
 */
static tchStatus tch_threadTerminate(tch_threadId thread,tchStatus err){
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_DESTROY,(uint32_t) thread,err);
	}
}

/***
 *
 */
static tch_threadId tch_threadSelf(){
	return (tch_threadId) tch_currentThread;
}

static tchStatus tch_threadSleep(){
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_SLEEP,0,0);
	}
}


static tchStatus tch_threadYield(uint32_t millisec){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,tchErrorISR);
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_YIELD,millisec,0);
	}
}

static tchStatus tch_threadJoin(tch_threadId thread,uint32_t timeout){
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,tchErrorISR);
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_JOIN,(uint32_t) thread,timeout);
	}
}


static void tch_threadSetPriority(tch_threadId id,tch_thread_prior nprior){
	if(nprior > Unpreemtible)
		return;
	getThreadHeader(id)->t_prior = nprior;
}

static tch_thread_prior tch_threadGetPriorty(tch_threadId id){
	return getThreadHeader(id)->t_prior;
}

static void* tch_threadGetArg(){
	return getThreadHeader(tch_currentThread)->t_arg;
}

static BOOL tch_threadIsRoot(){
	return tch_currentThread->t_state & THREAD_ROOT_BIT;
}


__attribute__((naked))static void __tch_thread_entry(tch_thread_header* thr_p,tchStatus status){

#ifdef MFEATURE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif
	thr_p->t_state = RUNNING;
	tchStatus res = thr_p->t_fn(tch_rti);
	tch_port_enterSv(SV_THREAD_DESTROY,(uint32_t) thr_p,status);
}


__attribute__((naked)) void __tch_kernel_atexit(tch_threadId thread,int status){
	// destroy & release used system resources
	tch_mem_ix* mem = NULL;
	tch_thread_header* th_p = getThreadHeader(thread);
	tch_thread_header* ch_p = NULL;
	uStdLib->stdio->iprintf("\rThread (%s) Exit with (%d)\n",getThreadHeader(thread)->t_name,status);
	uMem->forceRelease(thread);
	shMem->forceRelease(thread);
	if(th_p->t_flag & THREAD_ROOT_BIT){
		while(!tch_listIsEmpty(th_p->t_refNode.childs)){
			ch_p = (tch_thread_header*)((uint32_t) tch_listDequeue(th_p->t_refNode.childs) - 3 * sizeof(tch_lnode_t));
			if(ch_p){
				Thread->terminate(ch_p,status);
				Thread->join(ch_p,osWaitForever);        // wait child termination
				uStdLib->stdio->iprintf("\rThread (%s) Terminated ",ch_p->t_name);
			}
		}
		tch_listRemove((tch_lnode_t*)&tch_procList,&th_p->t_childNode);
		mem = (tch_mem_ix*)kMem;
	}else{
		tch_listRemove((tch_lnode_t*)getThreadHeader(th_p->t_refNode.root)->t_refNode.childs,&th_p->t_childNode);
		mem = (tch_mem_ix*)uMem;
		tch_currentThread = tch_currentThread->t_refNode.root;
	}
	mem->free(getThreadHeader(thread)->t_chks);
	tch_port_enterSv(SV_THREAD_TERMINATE,(uint32_t) thread,status);
}

