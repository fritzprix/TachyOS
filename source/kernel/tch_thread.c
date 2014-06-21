/*
 * tch_thread.c
 *
 *  Created on: 2014. 6. 17.
 *      Author: innocentevil
 */

#include "tch.h"
#include "lib/tch_absdata.h"



typedef struct _tch_thread_header_t tch_thread_header;

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
static void tch_threadStart(tch* _sys,tch_thread_id thread);
static osStatus tch_threadTerminate(tch* _sys,tch_thread_id thread);
static tch_thread_id tch_threadGetCurrent(tch* _sys);
static osStatus tch_threadSleep(tch* _sys,int millisec);
static osStatus tch_threadJoin(tch* _sys,tch_thread_id thread);
static void tch_threadSetPriority(tch* _sys,tch_thread_prior nprior);
static tch_thread_prior tch_threadGetPriorty(tch* _sys);


/**
 * private function
 */

static void __tch_threadEntry() __attribute__((naked));
static void __tch_onTerminated() __attribute__((naked));




struct _tch_thread_header_t {
	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_thread_routine          t_fn;
	const char*                 t_name;
	void*                       t_arg;
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	uint32_t                    t_status;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint32_t                    t_id;
	uint64_t                    t_to;
	tch_thread_context*         t_ctx;
} __attribute__((aligned(4)));


static tch_thread_ix tch_threadix = {
		tch_threadCreate,
		tch_threadStart,
		tch_threadTerminate,
		tch_threadGetCurrent,
		tch_threadSleep,
		tch_threadJoin,
		tch_threadSetPriority,
		tch_threadGetPriorty
};


const tch_thread_ix* Thread = &tch_threadix;

tch_thread_id tch_threadCreate(tch* _sys,tch_thread_cfg* cfg,void* arg){
	uint32_t* sptop = (uint32_t*)cfg->_t_stack + cfg->t_stackSize;             /// peek stack top pointer
	tch_port_ix* port = ((tch_prototype*) _sys)->tch_port;

	/**
	 * thread initialize from configuration type
	 */
	tch_thread_header* thread_p = ((tch_thread_header*) sptop - 1);
	thread_p->t_arg = arg;
	thread_p->t_fn = cfg->_t_routine;
	thread_p->t_name = cfg->_t_name;
	thread_p->t_prior = thread_p->t_svd_prior = cfg->t_proior;


	thread_p->t_ctx = ((tch_thread_context*)thread_p - 2);                      /**
	                                                                             *  thread context will be saved on 't_ctx'
	                                                                             *  initial sp is located in 2 context table offset below thread pointer
	                                                                             *  and context is manipulted to direct thread to thread start point
	                                                                             */
	port->_makeInitialContext(thread_p->t_ctx,__tch_threadEntry);                // manipulate initial context of thread

	thread_p->t_to = 0;
	thread_p->t_id = (uint32_t) thread_p;
	thread_p->t_lckCnt = 0;
	thread_p->t_listNode.next = thread_p->t_listNode.prev = NULL;
	tch_genericQue_Init(&thread_p->t_joinQ);

	return (tch_thread_id) thread_p;

}

void tch_threadStart(tch* _sys,tch_thread_id thread){
	tch_port_ix* tch_port = ((tch_prototype*) _sys)->tch_port;
	if(__get_IPSR()){
		// isr mode
		tch_port->_enterSvFromIsr();
	}else{
		// thread mode
	}
}

osStatus tch_threadTerminate(tch* _sys,tch_thread_id thread){

}

tch_thread_id tch_threadGetCurrent(tch* _sys){

}

osStatus tch_threadSleep(tch* _sys,int millisec){

}

osStatus tch_threadJoin(tch* _sys,tch_thread_id thread){

}

void tch_threadSetPriority(tch* _sys,tch_thread_prior nprior){

}

tch_thread_prior tch_threadGetPriorty(tch* _sys){

}


static void __tch_threadEntry() {
	// kernel unlock
	// invoke thread routine
	// invoke thread termination callback
}

static void __tch_onTerminated() {

}
