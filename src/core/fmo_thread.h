/*
 * fmo_thread.h
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */

#ifndef FMO_THREAD_H_
#define FMO_THREAD_H_

#include "port/tch_stdtypes.h"
#include "port/cortex_v7m_port.h"
#include "../util/generic_data_types.h"




#ifndef THR_TRACE_SIZE
#define THR_TRACE_SIZE    5
#endif


typedef void *(*tch_thread_routine_t)(void* arg);
typedef struct _tchthread_queue tchThread_queue;
typedef struct _tchthread_t tchThread_t;

typedef enum {
	exc_RESET = 1,exc_NMI = 2,exc_HardFault = 3, exc_MemManage = 4, exc_BusFault = 5,exc_UsageFault = 6, exc_SVCall = 11, exc_DebugMonitor = 12, exc_PendSV = 14, exc_SysTick = 15
}exc_id;

typedef struct _exc_trace_t exc_trace;

struct _exc_boundary_info_t{
	tchThread_t*         cthread;
	BOOL                nonbase;
	exc_id              excid;
	arm_exc_stack*      exc_stackp;
	arm_exc_stack       exc_stack;
	arm_exc_stack       exc_outter_stack;
	uint64_t            ts;
};

struct _exc_trace_t {
	struct _exc_boundary_info_t entry_info;
	struct _exc_boundary_info_t exit_info;
};

/*
void tch_thQue_Init(tchThread_queue* queue);
void tch_thQue_enqueue_byprior(tchThread_queue* queue,tchThread_t* thread);
void tch_thQue_enqueue_byTo(tchThread_queue* queue,tchThread_t* thread);
void tch_thQue_enqueueAhead(tchThread_queue* queue,tchThread_t* thread);
void tch_thQue_enqueueTail(tchThread_queue* queue,tchThread_t* thread);
tchThread_t* tch_thQue_dequeue(tchThread_queue* queue);
int tch_thQue_remove(tchThread_queue* queue,tchThread_t* thread);
*/


/**
 * Macro Variable
 */

#ifndef THREAD_TIME_SLOT
#define THREAD_TIME_SLOT 10
#endif

/**
 * Macro Variable
 */

#define THREAD_STATUS_ACTIVE       (uint32_t) (1 << 0)
#define THREAD_STATUS_PEND         (uint32_t) (1 << 1)
#define THREAD_STATUS_WAIT         (uint32_t) (1 << 2)
#define THREAD_STATUS_DEACTIVE     (uint32_t) (1 << 3)  // never started
#define THREAD_STATUS_READY        (uint32_t) (1 << 4)
#define THREAD_STATUS_TERMINATED   (uint32_t) (1 << 5)
#define THREAD_STATUS_MASK         (THREAD_STATUS_ACTIVE |\
		                            THREAD_STATUS_DEACTIVE|\
		                            THREAD_STATUS_PEND|\
		                            THREAD_STATUS_WAIT)

#define THREAD_FLAG_START          (uint32_t) (1 << 0)
#define THREAD_QUEUE_INIT          {NULL,NULL,0}


/**
 * Macro Function
 */


#define SET_THREAD_STATUS(th_p,nstate) {\
	th_p->t_status &= ~THREAD_STATUS_MASK;\
	th_p->t_status |= nstate;\
}

#define GET_THREAD_STATUS(th_p) (th_p->t_status & THREAD_STATUS_MASK)
#define SET_THREAD_STARTED(th_p) (th_p->t_flags |= THREAD_FLAG_START)
#define CLEAR_THREAD_STARTED(th_p) (th_p->t_flags &= ~(THREAD_FLAG_START))
#define IS_THREAD_STARTED(th_p) ((th_p->t_flags & THREAD_FLAG_START) != 0)
#define THREAD_ROUTINE(fn)       void* fn(void* arg)



struct _tchthread_queue{
	tchThread_t* entry;
	tchThread_t* tail;
	uint32_t cnt;
};

struct _tchthread_time_t{
	uint8_t  flag;
	uint64_t time_exp;
};

struct _tchthread_t{
	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_thread_routine_t        t_fn;
	const char*                t_name;
	void*                      t_arg;
	uint32_t                   t_tslot;
	uint32_t                   t_status;
	uint32_t                   t_flags;
	uint32_t                   t_prior;
	uint32_t                   t_svd_prior;
	uint32_t                   t_id;
	uint64_t                   t_to;
	fmo_thr_cntx               t_tctx;
#ifdef THR_TRACE_ENABLE
	exc_trace                  t_trace[THR_TRACE_SIZE];
	uint8_t                    t_traceIdx;
#endif
}__attribute__((aligned (4)));


tchThread_t* tchThread_create(uint32_t* th_spb,uint32_t size,void* (*fn)(void*),uint32_t t_prior,const char* tname);
BOOL tchThread_start(tchThread_t* thread);
BOOL tchThread_sleep(uint32_t milliSec);
BOOL tchThread_wait(tch_genericList_queue_t* queue);
BOOL tchThread_wake(tch_genericList_queue_t* queue);
BOOL tchThread_join(tchThread_t* jtarget);
tchThread_t* tchThread_getCurrent();


#endif /* FMO_THREAD_H_ */
