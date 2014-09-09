/*
 * tch_ktypes.h
 *
 *  Created on: 2014. 8. 9.
 *      Author: innocentevil
 */

#ifndef TCH_KTYPES_H_
#define TCH_KTYPES_H_

#include "tch.h"


#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_kernel_instance{
	tch                     tch_api;
} tch_kernel_instance;




typedef enum tch_thread_state_t {
	PENDED = 1,                              // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = 2,                             // state in which thread occupies cpu
	READY = 3,
	WAIT = 4,                                // state in which thread wait for event (wake)
	SLEEP = 5,                               // state in which thread is yield cpu for given amount of time
	TERMINATED = -1                          // state in which thread has finished its task
} tch_thread_state;


typedef struct tch_signal_t {
	int32_t                 match_target;
	int32_t                 signal;
	tch_lnode_t             sig_wq;
}tch_signal;



typedef struct tch_thread_header {
	tch_lnode_t                 t_schedNode;   ///<extends genericlist node class
	tch_lnode_t                 t_waitNode;
	tch_lnode_t                 t_joinQ;      ///<thread queue to wait for this thread's termination
	tch_lnode_t*                t_waitQ;      ///<reference to wait queue in which this thread is waiting
	tch_thread_routine          t_fn;         ///<thread function pointer
	const char*                 t_name;       ///<thread name /* currently not implemented */
	void*                       t_arg;        ///<thread arg field
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	tch_thread_state            t_state;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint64_t                    t_to;
	void*                       t_ctx;
	tch_signal                  t_sig;
	tchStatus                   t_kRet;
	uint32_t                    t_chks;
} tch_thread_header   __attribute__((aligned(8)));

typedef struct tch_thread_queue{
	tch_lnode_t             thque;
} tch_thread_queue;


typedef struct tch_async_cb_t {
	tch_lnode_t                     ln;
	int (*fn)(uint32_t,void*);
	void*                           arg;
	volatile uint8_t                status;
	uint8_t                         prior;
	tch_thread_queue                wq;
}tch_async_cb;



typedef struct tch_mtx_cb {
	uint32_t            state;
	uint32_t            key;
	tch_thread_queue    que;
	void*               own;
	tch_thread_prior    svdPrior;
}tch_mtx;


#define SV_EXIT_FROM_SV                  ((uint32_t) 0x02)

#define SV_SIG_MATCH                     ((uint32_t) 0x14)
#define SV_SIG_WAIT                      ((uint32_t) 0x15)


#define SV_THREAD_START                  ((uint32_t) 0x20)              ///< Supervisor call id for starting thread
#define SV_THREAD_TERMINATE              ((uint32_t) 0x21)              ///< Supervisor call id for terminate thread      /* Not Implemented here */
#define SV_THREAD_SLEEP                  ((uint32_t) 0x22)              ///< Supervisor call id for yeild cpu for specific  amount of time
#define SV_THREAD_JOIN                   ((uint32_t) 0x23)              ///< Supervisor call id for wait another thread is terminated
#define SV_THREAD_SUSPEND                ((uint32_t) 0x24)
#define SV_THREAD_RESUME                 ((uint32_t) 0x25)
#define SV_THREAD_RESUMEALL              ((uint32_t) 0x26)

#define SV_MEM_MALLOC                    ((uint32_t) 0x27)
#define SV_MEM_FREE                      ((uint32_t) 0x28)

#define SV_ASYNC_START                   ((uint32_t) 0x2A)               ///< Supervisor call id to start async task
#define SV_ASYNC_BLSTART                 ((uint32_t) 0x2B)               ///< Supervisor call id to start async task with blocking current execution
#define SV_ASYNC_NOTIFY                  ((uint32_t) 0x2C)               ///< Supervisor call id to notify async task result

#define SV_MSGQ_PUT                      ((uint32_t) 0x2D)               ///< Supervisor call id to put msg to msgq
#define SV_MSGQ_GET                      ((uint32_t) 0x2E)               ///< Supervisor call id to get msg from msgq
#define SV_MSGQ_DESTROY                  ((uint32_t) 0x2F)               ///< Supervisro call id to destoy msgq

#define SV_MEMP_ALLOC                    ((uint32_t) 0x30)               ///< Supervisor call id to allocate memory chunk form mem pool
#define SV_MEMP_FREE                     ((uint32_t) 0x31)               ///< Supervisor call id to free memory chunk into mem pool

#define SV_MAILQ_ALLOC                   ((uint32_t) 0x32)               ///< Supervisor call id to allocate mail
#define SV_MAILQ_FREE                    ((uint32_t) 0x33)               ///< Supervisor call id to free mail
#define SV_MAILQ_DESTROY                 ((uint32_t) 0x34)               ///< Supervisor call id to destroy mailq




#ifdef __cplusplus
}
#endif

#endif /* TCH_KTYPES_H_ */
