/*
 * tch_ktypes.h
 *
 *  Created on: 2014. 8. 9.
 *      Author: innocentevil
 */



#ifndef TCH_KTYPES_H_
#define TCH_KTYPES_H_

#include "tch.h"
#include "tch_list.h"



#if defined(__cplusplus)
extern "C" {
#endif


typedef enum tch_thread_state_t {
	PENDED =  ((int8_t) 1),                              // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = ((int8_t) 2),                             // state in which thread occupies cpu
	READY =   ((int8_t) 3),
	WAIT =    ((int8_t) 4),                                // state in which thread wait for event (wake)
	SLEEP =   ((int8_t) 5),                               // state in which thread is yield cpu for given amount of time
	DESTROYED =  ((int8_t) 6),                           // state in which thread is about to be destroyed (sheduler detected this and route thread to destroy routine)
	TERMINATED = ((int8_t) -1)                         // state in which thread has finished its task
} tch_thread_state;


typedef struct tch_sys_task_t tch_sysTask;
typedef void (*tch_sysTaskFn)(int id,const tch* env,void* arg);
typedef struct tch_thread_header_t tch_thread_header;
typedef struct tch_uobj_t tch_uobj;
typedef tchStatus (*tch_uobjDestr)(tch_uobj* obj);



struct tch_uobj_t {
	tchStatus (*destructor)(tch_uobj* obj);
};



struct tch_sys_task_t {
	tch_sysTaskFn          fn;
	tch_thread_prior       prior;
	void*                  arg;
	uint8_t                status;
	int                    id;
}__attribute__((packed));



typedef struct tch_thread_queue{
	tch_lnode_t             thque;
} tch_thread_queue;






struct tch_thread_header_t {
	tch_lnode_t                 t_schedNode;   ///<thread queue node to be scheduled
	tch_lnode_t                 t_waitNode;    ///<thread queue node to be blocked
	tch_lnode_t                 t_joinQ;      ///<thread queue to wait for this thread's termination
	tch_lnode_t                 t_childNode;   ///<thread queue node to iterate child thread
	tch_lnode_t*                t_waitQ;      ///<reference to wait queue in which this thread is waiting
	tch_thread_routine          t_fn;         ///<thread function pointer
	const char*                 t_name;       ///<thread name
	void*                       t_arg;        ///<thread arg field
	uint32_t                    t_tslot;      /// time slot for round robin scheduling (currently not used)
	tch_thread_state            t_state;      /// thread state
	uint8_t                     t_flag;       /// flag for dealing with attributes of thread
	uint8_t                     t_lckCnt;     /// lock count to know whether  restore original priority
	uint8_t                     t_prior;      /// priority
	uint64_t                    t_to;         /// timeout value for pending operation
	void*                       t_ctx;        /// ptr to thread saved context (stack pointer value)
	tchStatus                   t_kRet;       /// kernel return value
	tch_memId                   t_mem;        /// heap handle
	tch_lnode_t                 t_ualc;       /// allocation list for usr heap
	tch_lnode_t                 t_shalc;      /// allocation list for shared heap
	union {
		tch_thread_header* root;
		tch_lnode_t*       childs;
	} t_refNode;
	struct _reent               t_reent;      /// reentrant struct used by c standard library
	uint32_t*                   t_chks;       /// checksum for integrity check
} __attribute__((aligned(8)));



#define SV_EXIT_FROM_SV                  ((uint32_t) 0x02)


#define SV_THREAD_START                  ((uint32_t) 0x20)              ///< Supervisor call id for starting thread
#define SV_THREAD_TERMINATE              ((uint32_t) 0x21)              ///< Supervisor call id for terminate thread      /* Not Implemented here */
#define SV_THREAD_SLEEP                  ((uint32_t) 0x22)              ///< Supervisor call id for yeild cpu for specific  amount of time
#define SV_THREAD_JOIN                   ((uint32_t) 0x23)              ///< Supervisor call id for wait another thread is terminated
#define SV_THREAD_SUSPEND                ((uint32_t) 0x24)
#define SV_THREAD_RESUME                 ((uint32_t) 0x25)
#define SV_THREAD_RESUMEALL              ((uint32_t) 0x26)
#define SV_THREAD_DESTROY                ((uint32_t) 0x27)

#define SV_MSGQ_PUT                      ((uint32_t) 0x2D)               ///< Supervisor call id to put msg to msgq
#define SV_MSGQ_GET                      ((uint32_t) 0x2E)               ///< Supervisor call id to get msg from msgq
#define SV_MSGQ_DESTROY                  ((uint32_t) 0x2F)               ///< Supervisro call id to destoy msgq

#define SV_MEMP_ALLOC                    ((uint32_t) 0x30)               ///< Supervisor call id to allocate memory chunk form mem pool
#define SV_MEMP_FREE                     ((uint32_t) 0x31)               ///< Supervisor call id to free memory chunk into mem pool

#define SV_MAILQ_ALLOC                   ((uint32_t) 0x32)               ///< Supervisor call id to allocate mail
#define SV_MAILQ_FREE                    ((uint32_t) 0x33)               ///< Supervisor call id to free mail
#define SV_MAILQ_DESTROY                 ((uint32_t) 0x34)               ///< Supervisor call id to destroy mailq

#define SV_ASYNC_WAIT                    ((uint32_t) 0x36)               ///< Supervisor call id to post Async Kernel Task
#define SV_ASYNC_NOTIFY                  ((uint32_t) 0x37)               ///< Supervisor call id to notify completion of async kernel task
#define SV_ASYNC_DESTROY                 ((uint32_t) 0x38)               ///< Supervisor call id to destroy async


#define ROOT_THREAD                      ((tch_threadId) 1)

#ifdef __cplusplus
}
#endif

#endif /* TCH_KTYPES_H_ */
