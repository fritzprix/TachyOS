/*
 * tch.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 12.
 *      Author: innocentevil
 *
 *      This header defines tachyos kernel interface.
 */

#ifndef TCH_H_
#define TCH_H_
#ifdef __cplusplus
extern "C" {
#endif
#include "tch_TypeDef.h"


/*!
 * \mainpage
 *  this is main page
 */

/*!
 *  \brief Configuration Macro for CMSIS
 *
 *   These are macro for configuration of OS Feature which is compatible
 *   to CMSIS
 *
 */

#define osFeature_MainThread   1       ///< main thread      1=main can be thread, 0=not available
#define osFeature_Pool         1       ///< Memory Pools:    1=available, 0=not available
#define osFeature_MailQ        1       ///< Mail Queues:     1=available, 0=not available
#define osFeature_MessageQ     1       ///< Message Queues:  1=available, 0=not available
#define osFeature_Signals      8       ///< maximum number of Signal Flags available per thread
#define osFeature_Semaphore    30      ///< maximum count for \ref osSemaphoreCreate function
#define osFeature_Wait         1       ///< osWait function: 1=available, 0=not available
#define osFeature_SysTick      1       ///< osKernelSysTick functions: 1=available, 0=not available
/// Timeout value.
/// \note MUST REMAIN UNCHANGED: \b osWaitForever shall be consistent in every CMSIS-RTOS.
#define osWaitForever     0xFFFFFFFF     ///< wait forever timeout value



/*!
 *  tachyos kernel interface
 */
typedef struct _tch_thread_ix_t tch_thread_ix;
typedef struct _tch_condvar_ix_t tch_condv_ix;
typedef struct _tch_mutex_ix_t tch_mtx_ix;
typedef struct _tch_semaph_ix_t tch_semaph_ix;
typedef struct _tch_signal_ix_t tch_signal_ix;
typedef struct _tch_timer_ix_t tch_timer_ix;
typedef struct _tch_msgque_ix_t tch_msgq_ix;
typedef struct _tch_mailbox_ix_t tch_mailq_ix;
typedef struct _tch_mpool_ix_t tch_mpool_ix;
typedef struct _tch_mem_ix_t tch_mem_ix;
typedef struct _tch_ustdl_ix_t tch_ustdlib_ix;
typedef struct _tch_async_ix_t tch_async_ix;

typedef struct tch_hal_t tch_hal;


typedef struct _tch_t {
	const tch_thread_ix* Thread;
	const tch_signal_ix* Sig;
	const tch_timer_ix* Timer;
	const tch_condv_ix* Condv;
	const tch_mtx_ix* Mtx;
	const tch_semaph_ix* Sem;
	const tch_msgq_ix* MsgQ;
	const tch_mailq_ix* MailQ;
	const tch_mpool_ix* Mempool;                ///< Operating System
	const tch_hal* Device;                      ///< Entry of Device Driver Handles
	const tch_mem_ix* Mem;
	const tch_async_ix* Async;
	const tch_ustdlib_ix* uStdLib;              ///< minimal set of c standard library (Wrapper Class)
} tch;




#include "sys/tch_mtx.h"
#include "sys/tch_sem.h"
#include "sys/tch_thread.h"
#include "sys/tch_sig.h"
#include "sys/tch_vtimer.h"
#include "sys/tch_condv.h"
#include "sys/tch_mpool.h"
#include "sys/tch_ipc.h"
#include "sys/tch_mem.h"
#include "sys/tch_async.h"
#include "sys/tch_nclib.h"
#include "hal/tch_hal.h"




/***
 * tachyos generic data interface
 */

#ifdef __cplusplus
}
#endif
#endif /* TCH_H_ */
