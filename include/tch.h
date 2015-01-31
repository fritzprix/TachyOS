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

/** \addtogroup API
 *  @{
 */

#ifndef TCH_H_
#define TCH_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "tch_TypeDef.h"

#include "tch_usart.h"
#include "tch_spi.h"
#include "tch_i2c.h"
#include "tch_adc.h"
#include "tch_timer.h"
#include "tch_gpio.h"


struct tch_hal_t{
	const tch_lld_usart* usart;
	const tch_lld_spi*   spi;
	const tch_lld_iic*   i2c;
	const tch_lld_adc*   adc;
	const tch_lld_gpio*  gpio;
	const tch_lld_timer* timer;
};
/**
 * \mainpage Tachyos
 * \copyright Copyright (C) 2014-2015 doowoong,lee  All rights reserved.
 * \section about_sec About Tachyos
 *  Tachyos is a RTOS based on Microkernel architecture.
 *  so that any idea or concept can be prototyped easily but without any constraints in performance, stability and power efficiency.
 *  For this motivation, all the base APIs (Kernel & HAL) are thread-safe and I/O operation performed under the HAL is synchronous to
 *  program context(which means to be blocking).
 *
 *  Tachyos Kernel is small in its binary size and memory footprint(Kernel 10KB ROM / 2KB RAM) though, it includes all essential kernel components
 *  like multi-threading, synchronization, communication and also supports some of POSIX api used frequently in development.
 *
 *
 *  \section feat_sec Detailed features
 *  - <b>Preemtive Scheduler</b>
 *  	- Support unlimited multi-thread
 *  	- Support Both Round-Robin & Cooperative scheduling
 *  - <b>Synchronization & Inter-thread-communication</b>
*  		- provides pthread-like synchronization API (Mutex, Semaphore, Condition Variable)
*  		- provides Message Queue & MailBox to communicate with another threads or receive message from hardware interrupt
 *  - <b>Multi-level power mode</b>
 *  	- provides low power API so that threads can explicitly control where & when to enter low power mode synchronous to execution context
 *  	- provides low power hook for application to maximize application power efficiency
 *  - <b>Support POSIX API scalable manner</b>
 *  	- POSIX API support can be configured in build time so that many posix compatible application can be easily ported from another platform while maintainng small binary footprint
 *  - <b>Provides Safe-Dynamic Allocation /wo MMU</b>
 *  	- prevent Heap Fragmentation which can harm stability of system
 *  	- prevent unintentional resource(memory, I/O peripheral) leakage in kernel level
 *
 *   */


#define tchWaitForever     0xFFFFFFFF     ///< wait forever timeout value
#define tch_assert(api,b,err) if(!b){api->Thread->terminate(api->Thread->self(),err);}
#define DECLARE_THREADROUTINE(fn)                    int fn(const tch* env)


typedef struct _tch_runtime_t tch;

/*!
 *  tachyos kernel interface
 */





struct _tch_thread_ix_t {
	/**
	 *  Create Thread Object
	 */
	tch_threadId (*create)(tch_threadCfg* cfg,void* arg);
	/**
	 *  Start New Thread
	 */
	tchStatus (*start)(tch_threadId thread);
	tchStatus (*terminate)(tch_threadId thread,tchStatus err);
	tch_threadId (*self)();
	tchStatus (*yield)(uint32_t millisec);
	tchStatus (*sleep)(uint32_t sec);
	tchStatus (*join)(tch_threadId thread,uint32_t timeout);
	void (*setPriority)(tch_threadId id,tch_thread_prior nprior);
	tch_thread_prior (*getPriorty)(tch_threadId tid);
	void* (*getArg)();
};

struct _tch_mutex_ix_t {
	tch_mtxId (*create)();
	tchStatus (*lock)(tch_mtxId mtx,uint32_t timeout);
	tchStatus (*unlock)(tch_mtxId mtx);
	tchStatus (*destroy)(tch_mtxId mtx);
};


struct _tch_semaph_ix_t {
	tch_semId (*create)(uint32_t count);
	tchStatus (*lock)(tch_semId sid,uint32_t timeout);
	tchStatus (*unlock)(tch_semId sid);
	tchStatus (*destroy)(tch_semId sid);
};

struct _tch_bar_ix_t {
	tch_barId (*create)();
	tchStatus (*wait)(tch_barId bar,uint32_t timeout);
	tchStatus (*signal)(tch_barId bar,tchStatus result);
	tchStatus (*destroy)(tch_barId bar);
};


struct _tch_systime_ix_t {
	tchStatus (*getLocaltime)(struct tm* tm);
	tchStatus (*setLocaltime)(struct tm* tm,const tch_timezone tz);
	uint64_t (*getCurrentTimeMills)();
	uint64_t (*uptimeMills)();
};




/*!
 * \brief posix-like condition variable API struct
 */
struct _tch_condvar_ix_t {

	/*! \brief create posix-like condition variable
	 *  \return initiated \ref tch_condvId
	 */
	tch_condvId (*create)();
	/*! \brief wait on condition variable with unlocking mutex
	 *  \param[in] condv
	 *  \param[in] lock
	 *  \param[in] timeout
	 */
	tchStatus (*wait)(tch_condvId condv,tch_mtxId lock,uint32_t timeout);
	tchStatus (*wake)(tch_condvId condv);
	tchStatus (*wakeAll)(tch_condvId condv);
	tchStatus (*destroy)(tch_condvId condv);
};



struct _tch_mpool_ix_t {
	tch_mpoolId (*create)(size_t sz,uint32_t plen);
	void* (*alloc)(tch_mpoolId mpool);
	void* (*calloc)(tch_mpoolId mpool);
	tchStatus (*free)(tch_mpoolId mpool,void* block);
	tchStatus (*destroy)(tch_mpoolId mpool);
};

struct _tch_signal_ix_t {
	int32_t (*set)(tch_threadId thread,int32_t signals);
	int32_t (*clear)(tch_threadId thread,int32_t signals);
	tchStatus (*wait)(int32_t signals,uint32_t millisec);
};

struct _tch_event_ix_t {
	tch_eventId (*create)();
	int32_t (*set)(tch_eventId ev,int32_t signals);
	int32_t (*clear)(tch_eventId ev,int32_t signals);
	tchStatus (*wait)(tch_eventId ev,int32_t signals,uint32_t millisec);
	tchStatus (*destroy)(tch_eventId ev);
};

/**
 *    CMSIS RTOS Compatible message queue
 */

struct _tch_msgque_ix_t {
	/**
	 *  Create and initiate Message Queue instance
	 *  ##Note : In CMSIS, this function actually has one more arguement, which is thread. however it is pointless
	 *           because there isn't any operation which is dedicated to a given single thread
	 *
	 *  @arg1 message queue definition
	 *  @return queue ID
	 *
	 */
	tch_msgqId (*create)(uint32_t len);

	/**
	 * put message in the message queue (for producer side)
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *  @arg1 message queue id
	 *  @arg2 msg to send
	 *  @arg3 timeout in millisecond
	 *  @return osOK  : the message is put into the queue.
	 *          osErrorResource  : no memory in the queue was available.
	 *          osErrorTimeoutResource  : no memory in the queue was available during the given time limit.
	 *          osErrorParameter  : a parameter is invalid or outside of a permitted
	 */
	tchStatus (*put)(tch_msgqId,uint32_t msg,uint32_t millisec);

	/**
	 * Get a Message or Wait for a Message from a Queue
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *   @arg1 message queue id
	 *   @arg2 timeout in millisecond
	 *   @return osOK: no message is available in the queue and no timeout was specified.
	 *           osEventTimeout: no message has arrived during the given timeout period.
	 *           osEventMessage: message received, value.p contains the pointer to message.
	 *           osErrorParameter: a parameter is invalid or outside of a permitted range.
	 */
	tchEvent (*get)(tch_msgqId,uint32_t millisec);

	uint32_t (*getLength)(tch_msgqId);

	/*!
	 * \brief destroy msg queue
	 */
	tchStatus (*destroy)(tch_msgqId);
};


struct _tch_mailbox_ix_t {
	tch_mailqId (*create)(uint32_t sz,uint32_t qlen);
	void* (*alloc)(tch_mailqId qid,uint32_t millisec,tchStatus* result);
	void* (*calloc)(tch_mailqId qid,uint32_t millisec,tchStatus* result);
	tchStatus (*put)(tch_mailqId qid,void* mail);
	tchEvent (*get)(tch_mailqId qid,uint32_t millisec);
	uint32_t (*getBlockSize)(tch_mailqId qid);
	uint32_t (*getLength)(tch_mailqId qid);
	tchStatus (*free)(tch_mailqId qid,void* mail);
	tchStatus (*destroy)(tch_mailqId qid);
};

struct _tch_mem_ix_t {
	void* (*alloc)(size_t size);
	void (*free)(void*);
	uint32_t (*avail)(void);
	tchStatus (*forceRelease)(tch_threadId thread);
	void (*printFreeList)(void);
	void (*printAllocList)(void);
};


#include "tch_nclib.h"

extern DECLARE_THREADROUTINE(main);


/***
 * tachyos generic data interface
 */

#ifdef __cplusplus
}
#endif
#endif /* TCH_H_ */
