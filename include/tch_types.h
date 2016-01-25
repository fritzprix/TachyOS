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
typedef void* tch_rendvId;
typedef void* tch_waitqId;
/*! \brief condition variable identifier
 */
typedef void* tch_condvId;
typedef void* tch_mpoolId;
typedef void* tch_msgqId;
typedef void* tch_mailqId;
typedef void* tch_memId;
typedef void* tch_eventTree;




typedef struct tch_thread_api tch_thread_api_t;
typedef struct tch_condvar_api tch_condvar_api_t;
typedef struct tch_mutex_api tch_mutex_api_t;
typedef struct tch_semaphore_api tch_semaphore_api_t;
typedef struct tch_time_api tch_time_api_t;
typedef struct tch_messageQ_api tch_messageQ_api_t;
typedef struct tch_mailQ_api tch_mailQ_api_t;
typedef struct tch_mempool_api tch_mempool_api_t;
typedef struct tch_malloc_api tch_malloc_api_t;
typedef struct tch_barrier_api tch_barrier_api_t;
typedef struct tch_rendezvu_api tch_rendezvu_api_t;
typedef struct tch_event_api tch_event_api_t;
typedef struct tch_module_api tch_module_api_t;
typedef struct tcn_dbg_api tch_dbg_api_t;


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

typedef struct tch_core_api {
	const tch_thread_api_t* Thread;
	const tch_event_api_t* Event;
	const tch_time_api_t* Time;
	const tch_condvar_api_t* Condv;
	const tch_mutex_api_t* Mtx;
	const tch_semaphore_api_t* Sem;
	const tch_barrier_api_t* Barrier;
	const tch_rendezvu_api_t* Rendezvous;
	const tch_messageQ_api_t* MsgQ;
	const tch_mailQ_api_t* MailQ;
	const tch_mempool_api_t* Mempool;
	const tch_malloc_api_t* Mem;
	const tch_dbg_api_t* Dbg;
	const tch_module_api_t* Module;
} tch_core_api_t;

/// Status code values returned by CMSIS-RTOS functions.
/// \note MUST REMAIN UNCHANGED: \b osStatus shall be consistent in every CMSIS-RTOS.
typedef enum  {
  tchOK                    =   0,			///< function completed; no error or event occurred.
  tchInterrupted		   =  -1,			///< thread is interrupted from waiting
  tchEventSignal           =  -2,			///< function completed; signal event occurred.
  tchEventMessage          =  -3,			///< function completed; message event occurred.
  tchEventMail             =  -4,			///< function completed; mail event occurred.
  tchEventTimeout          =  -5,			///< function completed; timeout occurred.
  tchErrorIllegalAccess	   =  -6,			///< illegal access to kernel functionality
  tchErrorMemoryLeaked	   =  -7,			///< memory leakage detected
  tchErrorParameter        =  -8,			///< parameter error: a mandatory parameter was missing or specified an incorrect object.
  tchErrorResource         =  -9,			///< resource not available: a specified resource was not available.
  tchErrorTimeoutResource  =  -10,			///< resource not available within given time: a specified resource was not available within the timeout period.
  tchErrorISR              =  -11,			///< not allowed in ISR context: the function cannot be called from interrupt service routines.
  tchErrorISRRecursive     =  -12,			///< function called multiple times from ISR with same object.
  tchErrorPriority         =  -13,			///< system cannot determine priority or thread has illegal priority.
  tchErrorNoMemory         =  -14,			///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
  tchErrorValue            =  -15,			///< value of a parameter is out of range.
  tchErrorIo               =  -16,			///< Error occurs in IO operation
  tchErrorStackOverflow    =  -17,			///< stack overflow error
  tchErrorHeapCorruption   =  -18,			/// <heap corrupted
  tchErrorOS               =  -19,			///< unspecified RTOS error: run-time error but no other error message fits.
  tch_status_reserved      =  (uint32_t) 0xFFFFFF    ///< prevent from enum down-size compiler optimization.
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

typedef uint64_t time_t;

typedef void*	alrm_Id;
typedef enum {
	alrmIntv_Year = ((uint8_t) 5),
	alrmIntv_Month = ((uint8_t) 4),
	alrmIntv_Week = ((uint8_t) 3),
	alrmIntv_DAY = ((uint8_t) 2),
	alrmIntv_HOUR = ((uint8_t) 1),
	alrmIntv_NONE = ((uint8_t) 0)
} alrmIntv;

struct tm {
	  int	tm_sec;
	  int	tm_min;
	  int	tm_hour;
	  int	tm_mday;
	  int	tm_mon;
	  int	tm_year;
	  int	tm_wday;
	  int	tm_yday;
	  int	tm_isdst;
};

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

typedef int (*tch_thread_routine)(const tch_core_api_t* env);

typedef struct thread_config {
	size_t				 stksz;
	size_t				 heapsz;
	tch_thread_routine	 entry;
	tch_threadPrior      priority;
	const char*          name;
}thread_config_t;



#if defined(__cplusplus)
}
#endif

#endif /* TCH_TYPEDEF_H_ */


