/*
 * tch_ktypes.h
 *
 *  Created on: 2014. 8. 9.
 *      Author: innocentevil
 */



#ifndef TCH_KTYPES_H_
#define TCH_KTYPES_H_

#include "tch.h"
#include "tch_ptypes.h"
#include "cdsl_dlist.h"
#include "cdsl_rbtree.h"



#if defined(__cplusplus)
extern "C" {
#endif


#define __TCH_STATIC_INIT  __attribute__((section(".data")))


typedef struct {
	int 			version_major;		// kernel version number
	int		 		version_minor;
	const char*		arch_name;
	const char*		pltf_name;
} tch_kernel_descriptor;


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
	PENDED =  ((int8_t) 1),                            // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = ((int8_t) 2),                            // state in which thread occupies cpu
	READY =   ((int8_t) 3),
	WAIT =    ((int8_t) 4),                            // state in which thread wait for event (wake)
	SLEEP =   ((int8_t) 5),                            // state in which thread is yield cpu for given amount of time
	DESTROYED =  ((int8_t) 6),                         // state in which thread is about to be destroyed (sheduler detected this and route thread to destroy routine)
	TERMINATED = ((int8_t) -1)                         // state in which thread has finished its task
} tch_threadState;

typedef void* tch_pageId;
typedef void (*tch_sysTaskFn)(int id,const tch* env,void* arg);
typedef struct tch_thread_kheader_s tch_thread_kheader;
typedef struct tch_thread_uheader_s tch_thread_uheader;



typedef struct tch_thread_queue{
	cdsl_dlistNode_t             thque;
} tch_thread_queue;

struct tch_mm {
	struct proc_dynamic* 	dynamic;			///< per process dynamic memory mangement struct
	struct mem_region* 		text_region;		///< per process memory region where text section is stored
	struct mem_region* 		bss_region;			///< per process memory region where bss section is stored
	struct mem_region* 		data_region;		///< per process memory region where data section is stored
	struct mem_region* 		stk_region;			///< per process memory region to be used as stack
	struct mem_region*		heap_region;		///< per process memory region to be used as heap
	cdsl_dlistNode_t		kobj_list;			///< per thread kobjects list
	cdsl_dlistNode_t		alc_list;
	cdsl_dlistNode_t		shm_list;
	pgd_t* 					pgd;
	paddr_t 				estk;
#define ROOT				((uint32_t) 1)
#define DYN					((uint32_t) 2)
	uint32_t				flags;
};

struct tch_thread_uheader_s {
	tch_thread_routine			fn;					///<thread function pointer
	rb_treeNode_t*				uobjs;				///<user object tree for tracking
	void*						cache;
	uword_t						kRet;				///<kernel return value
	const char*					name;				///<thread name
	void*						t_arg;				///<thread arg field
	tch_thread_kheader*			kthread;			///<pointer to kernel level thread header
	void*						heap;
	tch_condvId 				condv;
	tch_mtxId					mtx;
#ifdef __NEWLIB__									///<@NOTE : NEWLIBC will be replaced by tch_libc which is more suitable for low-cost embedded system
	struct _reent				reent;				///<reentrant struct used by LIBC
#endif
	uint32_t					chks;				///<check-sum for determine corruption of thread header
} __attribute__((aligned(8)));

struct tch_thread_kheader_s {
	cdsl_dlistNode_t				t_schedNode;	///<thread queue node to be scheduled
	cdsl_dlistNode_t				t_waitNode;		///<thread queue node to be blocked
	cdsl_dlistNode_t				t_joinQ;		///<thread queue to wait for this thread's termination
	cdsl_dlistNode_t				child_list;		///<thread queue node to iterate child thread
	cdsl_dlistNode_t				t_siblingLn;	///<linked list entry for added into child list
	cdsl_dlistNode_t*				t_waitQ;		///<reference to wait queue in which this thread is waiting
	void*							ctx;			///<ptr to thread saved context (stack pointer value)
	struct tch_mm					mm;				///<embedded memory management handle struct
	cdsl_dlistNode_t				t_palc;			///<allocation list for page
	cdsl_dlistNode_t				t_pshalc;		///<allocation list for shared heap
	cdsl_dlistNode_t				t_upshalc;
	uint32_t						tslot;			///<time slot for round robin scheduling (currently not used)
	uint32_t						permission;
	tch_threadState					state;			///<thread state
	uint8_t							flag;			///<flag for dealing with attributes of thread
	uint8_t							lckCnt;			///<lock count to know whether  restore original priority
	uint8_t							prior;			///<priority
	uint64_t						to;				///<timeout value for pending operation
	tch_thread_uheader*				uthread;		///<pointer to user level thread header
	tch_thread_kheader*				parent;
} __attribute__((aligned(8)));



#define SV_EXIT_FROM_SV                  ((uint32_t) 0x00)

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

#define SV_MTX_CREATE					 ((uint32_t) 0x40)
#define SV_MTX_LOCK						 ((uint32_t) 0x29)
#define SV_MTX_UNLOCK					 ((uint32_t) 0x2A)
#define SV_MTX_DESTROY					 ((uint32_t) 0x2B)

#define SV_CONDV_INIT					 ((uint32_t) 0x2C)
#define SV_CONDV_WAIT					 ((uint32_t) 0x2D)
#define SV_CONDV_WAKE					 ((uint32_t) 0x2E)
#define SV_CONDV_DEINIT					 ((uint32_t) 0x2F)

#define SV_MSGQ_INIT					 ((uint32_t) 0x30)
#define SV_MSGQ_PUT                      ((uint32_t) 0x31)              ///< Supervisor call id to put msg to msgq
#define SV_MSGQ_GET                      ((uint32_t) 0x32)              ///< Supervisor call id to get msg from msgq
#define SV_MSGQ_DEINIT                   ((uint32_t) 0x33)              ///< Supervisro call id to destoy msgq

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
