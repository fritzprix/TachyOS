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

#include "cmsis_os.h"
#include "port/acm4f/tch_port.h"
#include "hal/STM_CMx/tch_hal.h"

/****
 *  general macro type
 */




typedef enum {
	true = 1,false = !true
} BOOL;


/**
 *  Thread relevant type definition
 */
typedef void* tch_thread_id;
typedef struct _tch_thread_cfg_t tch_thread_cfg;
typedef void* (*tch_thread_routine)(void* arg);

typedef struct tch_msg{
	tch_thread_id thread;
	uint32_t      msg;
	void*        _arg;
} tch_msg;


typedef enum {
	Realtime = 5,
	High = 4,
	Normal = 3,
	Low = 2,
	Idle = 1
} tch_thread_prior;


/***
 *  mutex  types
 */
typedef void* tch_mtx_id;
typedef struct _tch_mtx_t tch_mtx;

/***
 *  semaphore  types
 */
typedef void* tch_sem_id;
typedef struct _tch_sem_t tch_sem;


/***
 *  condition variable types
 */
typedef void* tch_condv_id;
typedef struct _tch_condv_t tch_condv;





/**
 *  mempool tpyes
 */
typedef struct _tch_mpool_def_t tch_mpoolDef_t;
typedef void* tch_mpool_id;

/***
 *  message type
 */
typedef struct _tch_msgQue_def_t tch_msgQueDef_t;
typedef void* tch_msgQue_id;

/***
 * mail type
 */
typedef struct _tch_mailQue_def_t tch_mailQueDef_t;
typedef void* tch_mailQue_id;




/***
 *  tachyos kernel interface
 */
typedef struct _tch_thread_ix_t tch_thread_ix;
typedef struct _tch_condvar_ix_t tch_condv_ix;
typedef struct _tch_mutex_ix_t tch_mtx_ix;
typedef struct _tch_semaph_ix_t tch_semaph_ix;
typedef struct _tch_signal_ix_t tch_signal_ix;
typedef struct _tch_msgque_ix_t tch_msgq_ix;
typedef struct _tch_mailbox_ix_t tch_mbox_ix;
typedef struct _tch_mpool_ix_t tch_mpool_ix;



typedef struct _tch_t tch;
struct _tch_t {
	const tch_thread_ix* Thread;
	const tch_signal_ix* Sig;
	const tch_condv_ix* Condv;
	const tch_mtx_ix* Mtx;
	const tch_semaph_ix* Sem;
	const tch_msgq_ix* MsgQ;
	const tch_mbox_ix* MailQ;
	const tch_mpool_ix* Mempool;
	const tch_hal* Hal;
};




/***
 * tachyos generic data interface
 */

typedef struct _tch_streambuffer_t tch_streamBuffer;
typedef struct _tch_istream_t tch_istream;
typedef struct _tch_ostream_t tch_ostream;




struct _tch_thread_cfg_t {
	uint32_t             t_stackSize;
	tch_thread_routine  _t_routine;
	tch_thread_prior     t_proior;
	void*               _t_stack;
	const char*         _t_name;
};

/**
 * Thread Interface
 */


struct _tch_thread_ix_t {
	/**
	 *  Create Thread Object
	 */
	tch_thread_id (*create)(tch* sys,tch_thread_cfg* cfg,void* arg);
	/**
	 *  Start New Thread
	 */
	void (*start)(tch* sys,tch_thread_id thread);
	osStatus (*terminate)(tch* sys,tch_thread_id thread);
	tch_thread_id (*getCurrent)(tch* sys);
	osStatus (*sleep)(tch* sys,int millisec);
	osStatus (*join)(tch* sys,tch_thread_id thread);
	void (*setPriority)(tch* sys,tch_thread_prior nprior);
	tch_thread_prior (*getPriorty)(tch* sys);
};


struct _tch_signal_ix_t {
	int32_t (*set)(tch_thread_id thread,int32_t signals);
	int32_t (*clear)(tch_thread_id thread,int32_t signals);
	int32_t (*wait)(tch_thread_id thread,uint32_t millisec);
};


struct _tch_mutex_ix_t {
	tch_mtx_id (*create)(tch_mtx* mtx);
	osStatus (*lock)(tch_mtx_id mtx,uint32_t timeout);
	osStatus (*unlock)(tch_mtx_id mtx);
	osStatus (*destroy)(tch_mtx_id mtx);
};

struct _tch_semaph_ix_t {
	tch_sem_id (*create)(tch_sem* sem);
	osStatus (*lock)(tch_sem_id sid,uint32_t timeout);
	osStatus (*unlock)(tch_sem_id sid);
	osStatus (*destroy)(tch_sem_id sid);
};

struct _tch_condvar_ix_t {
	tch_condv_id (*create)(tch_condv* condv);
	BOOL (*wait)(tch_condv* condv,uint32_t timeout);
	osStatus (*wake)(tch_condv* condv);
	osStatus (*wakeAll)(tch_condv* condv);
	osStatus (*destroy)(tch_condv* condv);
};


/**
 *    CMSIS RTOS Compatible message queue
 */

struct _tch_mpool_ix_t {
	tch_mpool_id (*create)(const tch_mpoolDef_t* pool);
	void* (*alloc)(tch_mpool_id mpool);
	void* (*calloc)(tch_mpool_id mpool);
	osStatus (*free)(tch_mpool_id mpool,void* block);
};

struct _tch_msgque_ix_t {
	tch_msgQue_id (*initMsgQ)(const tch_msgQueDef_t* que);
	osStatus (*put)(tch_msgQue_id,uint32_t msg,uint32_t millisec);
	osEvent (*get)(tch_msgQue_id,uint32_t millisec);
};

struct _tch_mailbox_ix_t {
	tch_mailQue_id (*create)(const tch_mailQueDef_t* que);
	void* (*alloc)(tch_mailQue_id qid,uint32_t millisec);
	void* (*calloc)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*put)(tch_mailQue_id qid);
	osEvent (*get)(tch_mailQue_id qid,uint32_t millisec);
	osStatus (*free)(tch_mailQue_id qid,void* mail);
};



extern const tch_thread_ix* Thread;
extern const tch_signal_ix* Sig;
extern const tch_condv_ix* Condv;
extern const tch_mtx_ix* Mtx;
extern const tch_semaph_ix* Sem;
extern const tch_msgq_ix* MsgQ;
extern const tch_mbox_ix* MailQ;
extern const tch_mpool_ix* Mempool;
extern const tch_hal* Hal;

/****
 * global accessible error handling routine
 */
void tch_error_handler(BOOL dump,osStatus status) __attribute__((naked));

#endif /* TCH_H_ */
