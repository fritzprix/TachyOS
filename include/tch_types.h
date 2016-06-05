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

/*!
 * \addtogroup kernel_api
 * @{
 *
 * \brief TachyOS API for kernel layer
 */

#if defined(__cplusplus)
extern "C"{
#endif

#define MODULE_TYPE_GPIO				((uint32_t) 0)	///< module ID for GPIO, GPIO hal handle can be registered and looked up with this module Id
#define MODULE_TYPE_RTC					((uint32_t) 1)  ///< RTC hal module ID
#define MODULE_TYPE_DMA					((uint32_t) 2)  ///< DMA hal module ID
#define MODULE_TYPE_UART				((uint32_t) 3)  ///< UART hal module ID
#define MODULE_TYPE_IIC					((uint32_t) 4)  ///< IIC hal module ID
#define MODULE_TYPE_SPI					((uint32_t) 5)  ///< SPI hal module ID
#define MODULE_TYPE_TIMER				((uint32_t) 6)  ///< Timer hal module ID
#define MODULE_TYPE_ANALOGIN			((uint32_t) 7)  ///< ADC hal module ID
#define MODULE_TYPE_ANALOGOUT			((uint32_t) 8)  ///< DAC hal module ID
#define MODULE_TYPE_SDIO				((uint32_t) 9)  ///< SDIO hal module ID

/*!
 * \brief bitmap for representing module dependency or avaiability
 */
typedef struct module_map {
	uint64_t _map[8];  ///< bitmap pattern width of 512bit
}module_map_t;         ///< module map type

typedef void* tch_threadId;		///< thread ID type
typedef void* tch_eventId;      ///< Object ID for event handle
typedef void* tch_mtxId;        ///< Object ID for mutex handle
typedef void* tch_semId;        ///< Object ID for semaphore handle
typedef void* tch_barId;        ///< Object ID for barrier handle
typedef void* tch_timerId;      ///< Object ID for system timer handle
typedef void* tch_waitqId;      ///< Object ID for wait queue handle
typedef void* tch_condvId;      ///< Object ID for condition variable
typedef void* tch_mpoolId;      ///< Object ID for Memory Pool handle
typedef void* tch_msgqId;       ///< Object ID for Message Queue
typedef void* tch_mailqId;      ///< Object ID for Mail Queue
typedef void* tch_memId;        ///< Object ID for Dynamic Memory allocator
typedef void* alrm_Id;          ///< Object ID for alarm handle



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
typedef struct tch_waitq_api tch_waitq_api_t;
typedef struct tch_event_api tch_event_api_t;
typedef struct tch_module_api tch_module_api_t;
typedef struct tcn_dbg_api tch_dbg_api_t;


/*!
 * \brief application header which contains metadata and loading information
 */
struct application_header {
	uint64_t			vid;                ///< Company or organization ID
	uint64_t			appid;              ///< App ID
	uint32_t			ver;                ///< Version specifier
	uint32_t			permission;         ///< permission flags
	uint32_t			req_stksz;          ///< required stack size refered in load time
	uint32_t			req_heapsz;         ///< required heap size refered in load time
	void* 				stext;              ///< start address of text section
	void* 				etext;              ///< end address of text section
	void*				sbss;               ///< start address of bss section (Zero-initialized variable)
	void*				ebss;               ///< end address of bss section
	void*				sdata;              ///< start address of data section (Non-zero initialized variable e.g. static)
	void*				edata;              ///< end address of data section
	void*				entry;              ///< pointer to app entry
	uint64_t			chks;               ///< optional checksum
};

/*!
 * \brief Core API
 */
typedef struct tch_core_api {
	const tch_thread_api_t* Thread;      ///< Thread API  /ref tch_thread_api
	const tch_event_api_t* Event;
	const tch_time_api_t* Time;
	const tch_condvar_api_t* Condv;
	const tch_mutex_api_t* Mtx;
	const tch_semaphore_api_t* Sem;
	const tch_barrier_api_t* Barrier;
	const tch_waitq_api_t* WaitQ;
	const tch_messageQ_api_t* MsgQ;
	const tch_mailQ_api_t* MailQ;
	const tch_mempool_api_t* Mempool;
	const tch_malloc_api_t* Mem;
	const tch_dbg_api_t* Dbg;
	const tch_module_api_t* Module;
} tch_core_api_t;

/*!
 * \brief TachyOS result code
 */
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
  tchErrorOS               =  -19,			         ///< unspecified RTOS error: run-time error but no other error message fits.
  tch_status_reserved      =  (uint32_t) 0xFFFFFF    ///< prevent from enum down-size compiler optimization.
} tchStatus;

/*!
 * \brief UTC offset type
 */
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

typedef uint64_t time_t;           ///< type representing time
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


/*!
 * \brief Thread priority type
 */
typedef enum {
	Kernel = 7,            ///< Highest Priority Level, user thread can't have this priority
	Unpreemtible = 6,      ///< Highest Priority Level which user thread can have,
	Realtime = 5,          ///< Priority for the thread that has real-time requirement
	High = 4,              ///< High priority
	Normal = 3,            ///< Normal Priority
	Low = 2,               ///< Lowest Priority user thread can has
	Idle = 1               ///< Lowest Priority which is only for Idle thread
} tch_threadPrior;

/*!
 * \brief Thread entry function type
 */
typedef int (*tch_thread_routine)(const tch_core_api_t* env);

/*!
 * \brief Thread Configuration Type
 */
typedef struct thread_config {
	size_t				 stksz;        ///< thread stack size in bytes
	size_t				 heapsz;       ///< thread heap size in bytes
	tch_thread_routine	 entry;        ///< thread function \sa tch_thread_routine
	tch_threadPrior      priority;     ///< thread priority \sa tch_threadPrior
	const char*          name;         ///< thread name
}thread_config_t;


/*!
 * @}
 */

#if defined(__cplusplus)
}
#endif

#endif /* TCH_TYPEDEF_H_ */


