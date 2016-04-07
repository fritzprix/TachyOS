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
#include "cdsl_nrbtree.h"


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
typedef void (*tch_sysTaskFn)(int id,const tch_core_api_t* env,void* arg);
typedef struct tch_thread_kheader_s tch_thread_kheader;
typedef struct tch_thread_uheader_s tch_thread_uheader;



typedef struct tch_thread_queue{
	dlistEntry_t             thque;
} tch_thread_queue;

struct tch_mm {
	struct proc_dynamic* 	dynamic;			///< per process dynamic memory mangement struct
	struct mem_region* 		text_region;		///< per process memory region where text section is stored
	struct mem_region* 		bss_region;			///< per process memory region where bss section is stored
	struct mem_region* 		data_region;		///< per process memory region where data section is stored
	struct mem_region* 		stk_region;			///< per process memory region to be used as stack
	struct mem_region*		heap_region;		///< per process memory region to be used as heap
	dlistEntry_t            kobj_list;			///< per thread kobjects list
	dlistEntry_t            alc_list;
	dlistEntry_t            shm_list;
	pgd_t* 					pgd;
	paddr_t 				estk;
#define ROOT				((uint32_t) 1)
#define DYN					((uint32_t) 2)
	uint32_t				flags;
};

struct tch_thread_uheader_s {
	tch_thread_routine          fn;					///<thread function pointer
	nrbtreeNode_t*              uobjs;				///<user object tree for tracking
	void*                       cache;
	const char*                 name;				///<thread name
	void*                       t_arg;				///<thread arg field
	tch_thread_kheader*         kthread;			///<pointer to kernel level thread header
	void*                       heap;
	tch_condvId                 condv;
	tch_mtxId                   mtx;
	tch_core_api_t              ctx;
	uword_t                     kRet;				///<kernel return value
	uint32_t                    chks;				///<check-sum for determine corruption of thread header
} __attribute__((aligned(8)));

struct tch_thread_kheader_s {
	dlistNode_t                     t_schedNode;    ///<thread queue node to be scheduled
	dlistNode_t                     t_waitNode;     ///<thread queue node to be blocked
	dlistEntry_t                    t_joinQ;        ///<thread queue to wait for this thread's termination
	dlistEntry_t                    child_list;     ///<thread queue node to iterate child thread
	dlistNode_t                     t_siblingLn;    ///<linked list entry for added into child list
	dlistEntry_t                    lockables;      ///<linked list of locks that are locked by this thread
	dlistEntry_t*                   t_waitQ;        ///<reference to wait queue in which this thread is waiting
	void*							ctx;			///<ptr to thread saved context (stack pointer value)
	struct tch_mm					mm;				///<embedded memory management handle struct
	dlistNode_t                     t_palc;			///<allocation list for page
	dlistNode_t                     t_pshalc;		///<allocation list for shared heap
	dlistNode_t                     t_upshalc;
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



#define SV_EXIT_FROM_SWITCH                  ((uint32_t) -1)


#ifdef __cplusplus
}
#endif

#endif /* TCH_KTYPES_H_ */
