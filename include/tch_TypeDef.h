/*
 * tch_TypeDef.h
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */

#ifndef TCH_TYPEDEF_H_
#define TCH_TYPEDEF_H_

#include <stddef.h>
#include "tch_ptypes.h"

#if defined(__cplusplus)
extern "C"{
#endif


typedef struct _tch_thread_ix_t tch_thread_ix;
typedef struct _tch_condvar_ix_t tch_condv_ix;
typedef struct _tch_mutex_ix_t tch_mtx_ix;
typedef struct _tch_semaph_ix_t tch_semaph_ix;
typedef struct _tch_systime_ix_t tch_systime_ix;
typedef struct _tch_msgque_ix_t tch_msgq_ix;
typedef struct _tch_mailbox_ix_t tch_mailq_ix;
typedef struct _tch_mpool_ix_t tch_mpool_ix;
typedef struct _tch_mem_ix_t tch_mem_ix;
typedef struct _tch_ustdl_ix_t tch_ustdlib_ix;
typedef struct _tch_async_ix_t tch_async_ix;
typedef struct _tch_bar_ix_t tch_bar_ix;
typedef struct _tch_usig_ix_t tch_usignal_ix;
typedef struct tch_hal_t tch_hal;


typedef struct _tch_runtime_t {
	const tch_thread_ix* Thread;
	const tch_usignal_ix* uSig;
	const tch_systime_ix* Timer;
	const tch_condv_ix* Condv;
	const tch_mtx_ix* Mtx;
	const tch_semaph_ix* Sem;
	const tch_bar_ix* Barrier;
	const tch_msgq_ix* MsgQ;
	const tch_mailq_ix* MailQ;
	const tch_mpool_ix* Mempool;                ///< Operating System
	const tch_hal* Device;                      ///< Entry of Device Driver Handles
	const tch_mem_ix* Mem;
	const tch_async_ix* Async;
	const tch_ustdlib_ix* uStdLib;              ///< minimal set of c standard library (Wrapper Class)
}tch;

/// Status code values returned by CMSIS-RTOS functions.
/// \note MUST REMAIN UNCHANGED: \b osStatus shall be consistent in every CMSIS-RTOS.
typedef enum  {
  osOK                    =     0,       ///< function completed; no error or event occurred.
  osEventSignal           =  0x08,       ///< function completed; signal event occurred.
  osEventMessage          =  0x10,       ///< function completed; message event occurred.
  osEventMail             =  0x20,       ///< function completed; mail event occurred.
  osEventTimeout          =  0x40,       ///< function completed; timeout occurred.
  osErrorParameter        =  0x80,       ///< parameter error: a mandatory parameter was missing or specified an incorrect object.
  osErrorResource         =  0x81,       ///< resource not available: a specified resource was not available.
  osErrorTimeoutResource  =  0xC1,       ///< resource not available within given time: a specified resource was not available within the timeout period.
  osErrorISR              =  0x82,       ///< not allowed in ISR context: the function cannot be called from interrupt service routines.
  osErrorISRRecursive     =  0x83,       ///< function called multiple times from ISR with same object.
  osErrorPriority         =  0x84,       ///< system cannot determine priority or thread has illegal priority.
  osErrorNoMemory         =  0x85,       ///< system is out of memory: it was impossible to allocate or reserve memory for the operation.
  osErrorValue            =  0x86,       ///< value of a parameter is out of range.
  osErrorOS               =  0xFF,       ///< unspecified RTOS error: run-time error but no other error message fits.
  os_status_reserved      =  WORD_MAX    ///< prevent from enum down-size compiler optimization.
} tchStatus;


typedef void (*tch_sigFuncPtr)(int,uint32_t);

#define SIG_DFL ((tch_sigFuncPtr) 0)	/* Default action */
#define SIG_IGN ((tch_sigFuncPtr) 1)	/* Ignore action */
#define SIG_ERR ((tch_sigFuncPtr)-1)	/* Error return */

#define SIGKILL ((int)  0)
#define SIGINT  ((int)  1)
#define SIGSEGV ((int)  2)
#define SIGUSR  ((int)  3)


typedef struct  {
  tchStatus                 status;     ///< status code: event or error information
  union  {
    uint32_t                    v;     ///< message as 32-bit value
    void                       *p;     ///< message or mail as void pointer
    int32_t               signals;     ///< signal flags
  } value;                             ///< event value
  void* def;
} osEvent;


/***
 *  General Types
 */
typedef enum {	TRUE = ((uint8_t)(1 > 0)),FALSE = ((uint8_t)!TRUE)  } BOOL;
typedef enum {	ActOnSleep,NoActOnSleep }tch_PwrOpt;
typedef enum {	bSet = 1,  bClear = 0   }tch_bState;



/**
 * type definition for thread api
 */
typedef enum {
	KThread = 8,
	Unpreemtible = 6,
	Realtime = 5,
	High = 4,
	Normal = 3,
	Low = 2,
	Idle = 1
} tch_thread_prior;

typedef int (*tch_thread_routine)(const tch* env);
typedef struct _tch_thread_cfg_t {
	uint16_t             t_stackSize;
	uint16_t             t_heapSize;
	tch_thread_routine  _t_routine;
	tch_thread_prior     t_proior;
	const char*         _t_name;
}tch_threadCfg;



/**
 * type definition for mpool
 */
typedef struct _tch_mpoolDef_t{
	uint32_t count;
	uint32_t align;
	void*    pool;
} tch_mpoolDef_t;



/**
 * type definition for system timer
 */

typedef void* (*tch_timer_callback)(void* arg);
typedef enum {
	Once,Periodic
}tch_timer_type;

typedef struct _tch_timer_def_t {
	tch_timer_callback     fn;
}tch_timer_def;




#if defined(__cplusplus)
}
#endif

#endif /* TCH_TYPEDEF_H_ */


