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


#include "stm32f4xx.h"

/****
 *  general macro type
 */
#define NULL                           ((void*) 0 )


/****
 *  thread specific macro type
 */
#define THREAD_MIN_STACK_SIZE          ((uint8_t)  1 << 8)
#ifndef MAIN_STACK_SIZE
#define MAIN_STACK_SIZE                ((uint8_t)   1 << 9)
#endif

#ifndef IDLE_STACK_SIZE
#define IDLE_STACK_SIZE                (THREAD_MIN_STACK_SIZE)
#endif





/***
 *  tachos data structure
 */
typedef void* tch_thread_id;
typedef struct _tch_thread_cfg_t tch_thread_cfg;
typedef void* (*tch_thread_routine)(void* arg);
typedef void* tch_mtx;
typedef struct _tch_condv_t tch_condv;
typedef enum { Kernel = 5, High = 4, Normal = 3, Low = 2, Idle = 1} tch_thread_prior;



/***
 *  tachos kernel interface
 */
typedef struct _tch_thread_ix_t tch_thread_ix;
typedef struct _tch_condvar_ix_t tch_condv_ix;
typedef struct _tch_mutex_ix_t tch_mtx_ix;
typedef struct _tch_semaph_ix_t tch_semaph_ix;
typedef struct _tch_msgque_ix_t tch_msgq_ix;
typedef struct _tch_mailbox_ix_t tch_mbox_ix;

typedef enum {
	true = 1,false = !1
} BOOL;



/**
 * 1. Kernel Interface
 *   -> implemented in 'tch_kernel.c'
 *
 */

extern BOOL tch_kernelInit(void* arg);
extern BOOL tch_kernelStart(void* arg);
extern void tch_kernelSysTick(void);


struct _tch_thread_cfg_t {
	void*               _t_stack;
	uint32_t             t_stackSize;
	tch_thread_routine  _t_routine;
	tch_thread_prior     t_proior;
};

/**
 * Thread Interface
 */

struct _tch_thread_ix_t {
	/**
	 *  Create Thread Object
	 */
	tch_thread_id (*create)(tch_thread_cfg* cfg,void* arg);
	/**
	 *  Start New Thread
	 */
	void (*start)(tch_thread_id thread);
	void (*terminate)(tch_thread_id thread);
	tch_thread_id (*getCurrent)(void);
	void (*sleep)(int millisec);
	void (*join)(tch_thread_id thread);
	void (*setPriority)(tch_thread_prior nprior);
	tch_thread_prior (*getPriorty)();
};


struct _tch_mutex_ix_t {
	void (*initMutex)(tch_mtx* mtx);
	BOOL (*lock)(tch_mtx* mtx,uint32_t timeout);
	void (*unlock)(tch_mtx* mtx);
};

struct _tch_condvar_ix_t {
	void (*initCondv)(tch_condv* condv);
	BOOL (*wait)(tch_condv* condv,uint32_t timeout);
	void (*wake)(tch_condv* condv);
	void (*wakeAll)(tch_condv* condv);
};

struct _tch_msgque_ix_t {
	void (*initMsgQ)();
};


#endif /* TCH_H_ */
