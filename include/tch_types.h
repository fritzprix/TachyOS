/*
 * tch_TypeDef.h
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */

#ifndef TCH_TYPEDEF_H_
#define TCH_TYPEDEF_H_

#include <stddef.h>
#include <stdint.h>
#include <time.h>

#if defined(__cplusplus)
extern "C"{
#endif


#define MODULE_TYPE_GPIO				((uint32_t) 0)
#define MODULE_TYPE_RTC					((uint32_t) 1)
#define MODULE_TYPE_DMA					((uint32_t) 2)
#define MODULE_TYPE_UART				((uint32_t) 3)
#define MODULE_TYPE_IIC					((uint32_t) 4)
#define MODULE_TYPE_SPI					((uint32_t) 5)
#define MODULE_TYPE_TIMER				((uint32_t) 6)
#define MODULE_TYPE_ANALOGIN			((uint32_t) 7)
#define MODULE_TYPE_ANALOGOUT			((uint32_t) 8)


typedef struct module_map {
	uint64_t _map[8];
}module_map_t;

typedef void* tch_threadId;
typedef void* tch_eventId;
typedef void* tch_mtxId;
typedef void* tch_semId;
typedef void* tch_barId;
typedef void* tch_timerId;
/*! \brief condition variable identifier
 */
typedef void* tch_condvId;
typedef void* tch_mpoolId;
typedef void* tch_msgqId;
typedef void* tch_mailqId;
typedef void* tch_memId;
typedef void* tch_eventTree;




typedef struct tch_thread_ix_t tch_thread_ix;
typedef struct tch_condvar_ix_t tch_condv_ix;
typedef struct tch_mutex_ix_t tch_mtx_ix;
typedef struct tch_semaph_ix_t tch_semaph_ix;
typedef struct tch_time_ix_t tch_time_ix;
typedef struct tch_msgque_ix_t tch_msgq_ix;
typedef struct tch_mailbox_ix_t tch_mailq_ix;
typedef struct tch_mpool_ix_t tch_mpool_ix;
typedef struct tch_mem_ix_t tch_mem_ix;
typedef struct tch_bar_ix_t tch_bar_ix;
typedef struct tch_event_ix_t tch_event_ix;
typedef struct tch_service_ix_t tch_service_ix;


/**
 *  tachyos app post processor attach header in front of application binary
 */
struct application_header {
	uint64_t			vid;
	uint64_t			appid;
	uint32_t			ver;
	uint32_t			permission;
	uint32_t			req_stksz;
	uint32_t			req_heapsz;
	void* 				stext;
	void* 				etext;
	void*				sbss;
	void*				ebss;
	void*				sdata;
	void*				edata;
	void*				entry;
	uint64_t			chks;
};


typedef struct _tch_runtime_t {
	const tch_thread_ix* Thread;
	const tch_event_ix* Event;
	const tch_time_ix* Time;
	const tch_condv_ix* Condv;
	const tch_mtx_ix* Mtx;
	const tch_semaph_ix* Sem;
	const tch_bar_ix* Barrier;
	const tch_msgq_ix* MsgQ;
	const tch_mailq_ix* MailQ;
	const tch_mpool_ix* Mempool;
	const tch_mem_ix* Mem;
	const tch_service_ix* Service;
} tch;

/// Status code values returned by CMSIS-RTOS functions.
/// \note MUST REMAIN UNCHANGED: \b osStatus shall be consistent in every CMSIS-RTOS.
typedef enum  {
  tchOK                    =     0,       ///< function completed; no error or event occurred.
  tchInterrupted		   =  0x01,		  ///< thread is interrupted from waiting
  tchEventSignal           =  0x08,       ///< function completed; signal event occurred.
  tchEventMessage          =  0x10,       ///< function completed; message event occurred.
  tchEventMail             =  0x20,       ///< function completed; mail event occurred.
  tchEventTimeout          =  0x40,       ///< function completed; timeout occurred.
  tchErrorIllegalAccess	   =  0x42,		  ///< illegal access to kernel functionality
  tchErrorMemoryLeaked	   =  0x44,		  ///< memory leakage detected
  tchErrorParameter        =  0x80,       ///< parameter error: a mandatory parameter was missing or specified an incorrect object.
  tchErrorResource         =  0x81,       ///< resource not available: a specified resource was not available.
  tchErrorTimeoutResource  =  0xC1,       ///< resource not available within given time: a specified resource was not available within the timeout period.
  tchErrorISR              =  0x82,       ///< not allowed in ISR context: the function cannot be called from interrupt service routines.
  tchErrorISRRecursive     =  0x83,       ///< function called multiple times from ISR with same object.
  tchErrorPriority         =  0x84,       ///< system cannot determine priority or thread has illegal priority.
  tchErrorNoMemory         =  0x85,       ///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
  tchErrorValue            =  0x86,       ///< value of a parameter is out of range.
  tchErrorIo               =  0x87,       ///< Error occurs in IO operation
  tchErrorStackOverflow    =  0x88,       ///< stack overflow error
  tchErrorHeapCorruption   =  0x89,		  /// <heap corrupted
  tchErrorOS               =  0xFF,       ///< unspecified RTOS error: run-time error but no other error message fits.
  tch_status_reserved       =  0xFFFFFFFF    ///< prevent from enum down-size compiler optimization.
} tchStatus;

typedef enum {
	UTC_N12 = ((int8_t) -12),
	UTC_N11 = ((int8_t) -11),
	UTC_N10 = ((int8_t) -10),
	UTC_N9 =  ((int8_t) -9),
	UTC_N8 =  ((int8_t) -8),
	UTC_N7 =  ((int8_t) -7),
	UTC_N6 =  ((int8_t) -6),
	UTC_N5 =  ((int8_t) -5),
	UTC_N4 =  ((int8_t) -4),
	UTC_N3 =  ((int8_t) -3),
	UTC_N2 =  ((int8_t) -2),
	UTC_N1 =  ((int8_t) -1),
	UTC_0 =   ((int8_t) 0),
	UTC_P1 =  ((int8_t) 1),
	UTC_P2 =  ((int8_t) 2),
	UTC_P3 =  ((int8_t) 3),
	UTC_P4 =  ((int8_t) 4),
	UTC_P5 =  ((int8_t) 5),
	UTC_P6 =  ((int8_t) 6),
	UTC_P7 =  ((int8_t) 7),
	UTC_P8 =  ((int8_t) 8),
	UTC_P9 =  ((int8_t) 9),
	UTC_P10 = ((int8_t) 10),
	UTC_P11 = ((int8_t) 11),
	UTC_P12 = ((int8_t) 12),
	UTC_P13 = ((int8_t) 13),
	UTC_P14 = ((int8_t) 14)
}tch_timezone;


typedef struct mstat_t {
	size_t total;
	size_t used;
	size_t cached;
} mstat;

typedef struct  {
  tchStatus                 status;     ///< status code: event or error information
  union  {
    uint32_t                    v;     ///< message as 32-bit value
    void                       *p;     ///< message or mail as void pointer
    int32_t               signals;     ///< signal flags
  } value;                             ///< event value
  void* def;
} tchEvent;


/***
 *  General Types
 */
 /*
#ifndef BOOL
#define BOOL	uint8_t
#endif

#define TRUE  	((uint8_t) (1 > 0))
#define FALSE	!(TRUE)*/

typedef enum {	TRUE = ((uint8_t)(1 > 0)),FALSE = ((uint8_t)!TRUE)  } BOOL;
typedef enum {	ActOnSleep,NoActOnSleep }tch_PwrOpt;
typedef enum {	bSet = 1,  bClear = 0   }tch_bState;



/**
 * type definition for thread api
 */
typedef enum {
	Kernel = 7,
	Unpreemtible = 6,
	Realtime = 5,
	High = 4,
	Normal = 3,
	Low = 2,
	Idle = 1
} tch_threadPrior;

typedef int (*tch_thread_routine)(const tch* env);

typedef struct tch_user_mem_cfg_s {
	uint32_t 	stk_sz;				// user stack size
	uint32_t 	heap_sz;			// user heap size
	uint32_t 	pimg_sz;			// user process image size
	uint32_t*	u_mem;				// supplied user memory chunk for build thread context (only needed in child creation)
	uint32_t	u_memsz;			// size of supplied user memory (only needed in child creation)
} tch_userMemDef_t;

typedef struct thread_config {
	size_t				 stksz;
	size_t				 heapsz;
	tch_thread_routine	 entry;
	tch_threadPrior      priority;
	const char*          name;
}thread_config_t;



/**
 * type definition for mpool
 */
typedef struct _tch_mpoolDef_t{
	uint32_t count;
	uint32_t align;
	void*    pool;
} tch_mpoolDef_t;


typedef BOOL (*tch_eventHandler)(int ev_id,int ev_msg);

/**
 * type definition for system timer
 */

typedef void* (*tch_timer_callback)(void* arg);
typedef enum {
	Once,Periodic
}tch_timer_type;

typedef struct tch_timer_def_s {
	tch_timer_callback     fn;
}tch_timer_def;



#if defined(__cplusplus)
}
#endif

#endif /* TCH_TYPEDEF_H_ */


