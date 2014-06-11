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

#define NULL      ((void*) 0 )

typedef struct _tch_thread_t tch_Thread;
typedef struct _tch_thread_cfg_t tch_Thread_cfg;
typedef struct _tch_thread_ix_t tch_Thread_ix;
typedef struct _tch_mtx_t tch_mtx;
typedef struct _tch_condv_t tch_condv;
typedef enum { Kernel = 5, High = 4, Normal = 3, Low = 2, Idle = 1} tch_thread_prior;


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

/**
 * Thread Interface
 */

struct _tch_thread_ix_t {
	tch_Thread* (*create)(tch_Thread_cfg* cfg,void* arg);
	void (*start)(tch_Thread* thread);
	void (*terminate)(tch_Thread* thread);
	tch_Thread* (*getCurrent)(void);
	void (*wait)(tch_condv* condv);
	void (*sleep)(int millisec);
	void (*join)(tch_Thread* thread);
	void (*setPriority)()
};
extern tch_Thread* tch_thread_create(tch_Thread_cfg* cfg, void* arg);
extern tch_Thread* tch_thread_current(void);











#endif /* TCH_H_ */
