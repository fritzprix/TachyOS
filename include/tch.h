/*
 * tch.h
 *
 *  Created on: 2014. 6. 12.
 *      Author: innocentevil
 *
 *      This header defines tachyos kernel interface.
 */

#ifndef TCH_H_
#define TCH_H_

#include <cmsis_os.h>
#include <stddef.h>


/***
 *  General Types
 */
typedef enum {
	true = 1,false = !true
} BOOL;

typedef enum {
	ActOnSleep,NoActOnSleep
}tch_pwr_def;


/***
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

typedef struct tch_hal tch_hal;


typedef struct _tch_t {
	const tch_thread_ix* Thread;
	const tch_signal_ix* Sig;
	const tch_timer_ix* Timer;
	const tch_condv_ix* Condv;
	const tch_mtx_ix* Mtx;
	const tch_semaph_ix* Sem;
	const tch_msgq_ix* MsgQ;
	const tch_mailq_ix* MailQ;
	const tch_mpool_ix* Mempool;
	const tch_hal* Device;
} tch;






#include "kernel/tch_mtx.h"
#include "kernel/tch_sem.h"
#include "kernel/tch_thread.h"
#include "kernel/tch_sig.h"
#include "kernel/tch_vtimer.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_mpool.h"
#include "kernel/tch_ipc.h"




/***
 * tachyos generic data interface
 */

typedef struct _tch_streambuffer_t tch_streamBuffer;
typedef struct _tch_istream_t tch_istream;
typedef struct _tch_ostream_t tch_ostream;





/****
 * global accessible error handling routine
 */
void tch_error_handler(BOOL dump,osStatus status) __attribute__((naked));

#endif /* TCH_H_ */
