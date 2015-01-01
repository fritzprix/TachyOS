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
#define tch_assert(api,b,err) if(!b){api->Thread->terminate(api->Thread->self(),err);}
#define DECLARE_THREADROUTINE(fn)                    int fn(const tch* env)


typedef struct _tch_runtime_t tch;

/*!
 *  tachyos kernel interface
 */


typedef void* tch_threadId;
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
	tchStatus (*sleep)();
	tchStatus (*join)(tch_threadId thread,uint32_t timeout);
	void (*setPriority)(tch_threadId id,tch_thread_prior nprior);
	tch_thread_prior (*getPriorty)(tch_threadId tid);
	void* (*getArg)();
	BOOL (*isRoot)();
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

struct _tch_event_ix_t {
	tch_eventTree* (*createEventTree)();
	tchStatus (*listen)(tch_eventTree* self,tch_eventHandler ev_handler);
	tchStatus (*waitEvent)(tch_eventTree* self,int ev_id,uint32_t timeout);
	tchStatus (*raise)(tch_eventTree* self,int ev_id,int ev_msg);
	tchStatus (*raiseAll)(int ev_id,int ev_msg);
	void (*destroy)(tch_eventTree* self);
};



#include "sys/tch_nclib.h"
#include "hal/tch_hal.h"


extern DECLARE_THREADROUTINE(main);


/***
 * tachyos generic data interface
 */

#ifdef __cplusplus
}
#endif
#endif /* TCH_H_ */
