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

typedef struct {
	const char*          hw_vendor;
	const char*          hw_plf;
	const char*          build_time;
	uint32_t             major_ver;
	uint32_t             minor_ver;
} tch_bin_descriptor;

typedef struct tch_dynamic_bin_header_t {
	uint16_t b_type;
	uint32_t b_majorv;
	uint32_t b_minorv;
	uint32_t b_vendor;
	uint64_t b_appId;
	uint32_t b_key;
	uint32_t b_chk;
	uint32_t b_sz;
	uint32_t b_entry;
}* tch_dynamic_bin_header;



typedef enum {mSECOND = 0,SECOND = 1} tch_timeunit;

typedef enum tch_thread_state_t {
	PENDED =  ((int8_t) 1),                              // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = ((int8_t) 2),                             // state in which thread occupies cpu
	READY =   ((int8_t) 3),
	WAIT =    ((int8_t) 4),                                // state in which thread wait for event (wake)
	SLEEP =   ((int8_t) 5),                               // state in which thread is yield cpu for given amount of time
	DESTROYED =  ((int8_t) 6),                           // state in which thread is about to be destroyed (sheduler detected this and route thread to destroy routine)
	TERMINATED = ((int8_t) -1)                         // state in which thread has finished its task
} tch_threadState;

typedef void* tch_pageId;
typedef struct tch_sys_task_t tch_sysTask;
typedef void (*tch_sysTaskFn)(int id,const tch* env,void* arg);
typedef struct tch_thread_kheader_s tch_thread_kheader;
typedef struct tch_thread_uheader_s tch_thread_uheader;

typedef struct tch_thread_kcb_s tch_thread_kcb;		///< protected thread control block which contains scheduling information / thread status & state etc,
typedef struct tch_thread_ucb_s tch_thread_ucb;		///< public thread control block which contains general data which is not critical for the system

typedef struct tch_thread_footer_t tch_thread_footer;
typedef struct tch_uobj_t tch_uobj;
typedef tchStatus (*tch_uobjDestr)(tch_uobj* obj);

typedef struct tch_errorDescriptor {
	int            errtype;
	int            errno;
	tch_threadId   subj;
}tch_errorDescriptor;

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


struct tch_uheap_handle_t {
	tch_memId		u_mem;
	tch_lnode_t		u_alc;
}__attribute__((packed));


struct tch_thread_uheader_s {
	tch_uobjDestr				t_destr;
	tch_thread_routine          t_fn;			///<thread function pointer
	struct tch_uheap_handle_t* 						t_heap;
	uword_t                     t_kRet;			///<kernel return value
	const char*                 t_name;			///<thread name
	void*                       t_arg;			///<thread arg field
	tch_thread_kheader*			t_kthread;		///<pointer to kernel level thread header
#ifdef __NEWLIB__
	struct _reent               t_reent;		///<reentrant struct used by c standard library
#endif
	uint32_t					t_chks;			///<check-sum for determine corruption of thread header
} __attribute__((aligned(8)));

struct tch_thread_kheader_s {
	tch_lnode_t                 t_schedNode;	///<thread queue node to be scheduled
	tch_lnode_t                 t_waitNode;		///<thread queue node to be blocked
	tch_lnode_t                 t_joinQ;		///<thread queue to wait for this thread's termination
	tch_lnode_t                 t_childLn;		///<thread queue node to iterate child thread
	tch_lnode_t					t_siblingLn;	///<linked list entry for added into child list
	tch_lnode_t*                t_waitQ;		///<reference to wait queue in which this thread is waiting
	void*                       t_ctx;			///<ptr to thread saved context (stack pointer value)
	void*						t_proc;			///<ptr to base address of process image
	tch_lnode_t					t_palc;			///<allocation list for page
	tch_lnode_t                 t_pshalc;		///<allocation list for shared heap
	tch_lnode_t					t_upshalc;
	uint32_t                    t_tslot;		///<time slot for round robin scheduling (currently not used)
	tch_threadState             t_state;		///<thread state
	uint8_t                     t_flag;			///<flag for dealing with attributes of thread
	uint8_t                     t_lckCnt;		///<lock count to know whether  restore original priority
	uint8_t                     t_prior;		///<priority
	uint64_t					t_to;			///<timeout value for pending operation
	tch_pageId					t_pgId;
	tch_thread_uheader*			t_uthread;		///<pointer to user level thread header
	tch_thread_kheader*			t_parent;
} __attribute__((aligned(8)));


struct tch_thread_kcb_s {
	tch_lnode_t			t_schedNode;
	tch_lnode_t			t_waitNode;
	tch_lnode_t			t_joinQ;
	tch_lnode_t			t_childNode;
	tch_lnode_t*		t_waitQ;
	void*				t_ctx;
	tchStatus			t_kRet;
	tch_memId			t_mem;
	tch_lnode_t			t_ualc;
	tch_lnode_t			t_shalc;
	uint32_t			t_tslot;
	tch_threadState		t_state;
	uint8_t				t_flag;
	uint8_t				t_lckCnt;
	uint8_t				t_prior;
	uint32_t* 			t_chks;
	uint64_t			t_to;
	tch_thread_ucb*		t_ucb;
};

struct tch_thread_ucb_s {
	tch_uobjDestr		__destr;
	tch_thread_queue	childs;
	tch_threadId		parent;
	uint8_t*			__heap_entry;
	tch_thread_routine	t_fn;
	const char*			t_name;
	void*				t_arg;
	struct _reent		t_reent;
};

struct tch_thread_footer_t{
	tch_uobjDestr		__destr;
	tch_thread_queue	childs;
	tch_threadId		parent;
};



#define SV_EXIT_FROM_SV                  ((uint32_t) 0x02)

#define SV_EV_INIT						 ((uint32_t) 0x15)
#define SV_EV_UPDATE                     ((uint32_t) 0x16)              ///< Supervisor call id for setting / clearing event flag
#define SV_EV_WAIT                       ((uint32_t) 0x17)              ///< Supervisor call id for blocking to wait event
#define SV_EV_DEINIT                     ((uint32_t) 0x18)              ///< Supervisor call id for destroying event node

#define SV_THREAD_CREATE				 ((uint32_t) 0x1A)				///< Supervisor call id for creating thread
#define SV_THREAD_START                  ((uint32_t) 0x20)              ///< Supervisor call id for starting thread
#define SV_THREAD_TERMINATE              ((uint32_t) 0x21)              ///< Supervisor call id for terminate thread      /* Not Implemented here */
#define SV_THREAD_YIELD                  ((uint32_t) 0x22)              ///< Supervisor call id for yeild cpu for specific  amount of time
#define SV_THREAD_JOIN                   ((uint32_t) 0x23)              ///< Supervisor call id for wait another thread is terminated
#define SV_THREAD_SUSPEND                ((uint32_t) 0x24)
#define SV_THREAD_RESUME                 ((uint32_t) 0x25)
#define SV_THREAD_RESUMEALL              ((uint32_t) 0x26)
#define SV_THREAD_DESTROY                ((uint32_t) 0x27)
#define SV_THREAD_SLEEP                  ((uint32_t) 0x28)               ///< Supervisor call id to put thread in low power stand-by mode

#define SV_MTX_LOCK						 ((uint32_t) 0x29)
#define SV_MTX_UNLOCK					 ((uint32_t) 0x2A)
#define SV_MTX_DESTROY					 ((uint32_t) 0x2B)

/*
#define SV_CONDV_INIT					 ((uint32_t) 0x2C)
#define SV_CONDV_WAIT					 ((uint32_t) 0x2D)
#define SV_CONDV_WAKE					 ((uint32_t) 0x2E)
#define SV_CONDV_DEINIT					 ((uint32_t) 0x2F)
*/

#define SV_MSGQ_INIT					 ((uint32_t) 0x30)
#define SV_MSGQ_PUT                      ((uint32_t) 0x31)               ///< Supervisor call id to put msg to msgq
#define SV_MSGQ_GET                      ((uint32_t) 0x32)               ///< Supervisor call id to get msg from msgq
#define SV_MSGQ_DEINIT                  ((uint32_t) 0x33)               ///< Supervisro call id to destoy msgq

#define SV_MEMP_ALLOC                    ((uint32_t) 0x34)               ///< Supervisor call id to allocate memory chunk form mem pool
#define SV_MEMP_FREE                     ((uint32_t) 0x35)               ///< Supervisor call id to free memory chunk into mem pool

#define SV_MAILQ_INIT					 ((uint32_t) 0x36)
#define SV_MAILQ_ALLOC                   ((uint32_t) 0x37)               ///< Supervisor call id to allocate mail
#define SV_MAILQ_FREE                    ((uint32_t) 0x38)               ///< Supervisor call id to free mail
#define SV_MAILQ_DEINIT                  ((uint32_t) 0x39)               ///< Supervisor call id to destroy mailq

#define SV_BAR_INIT 	                 ((uint32_t) 0x3A)               ///< Supervisor call id to post Async Kernel Task
#define SV_BAR_DEINIT   	             ((uint32_t) 0x3B)               ///< Supervisor call id to notify completion of async kernel task

#define SV_SHMEM_ALLOC				 	 ((uint32_t) 0x3C)
#define SV_SHMEM_FREE				  	 ((uint32_t) 0x3D)
#define SV_SHMEM_AVAILABLE			 	 ((uint32_t) 0x3E)

#define SV_HAL_ENABLE_ISR                ((uint32_t) 0xFD)
#define SV_HAL_DISABLE_ISR               ((uint32_t) 0xFC)




#ifdef __cplusplus
}
#endif

#endif /* TCH_KTYPES_H_ */
