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

#include "tch_types.h"

#include "tch_usart.h"
#include "tch_spi.h"
#include "tch_i2c.h"
#include "tch_adc.h"
#include "tch_timer.h"
#include "tch_gpio.h"
#include "tch_sdio.h"


/**
 * \mainpage TachyOS
 * \copyright 2014-2016 doowoong,lee  All rights reserved.
 * \section about_sec About TachyOS
 *  TachyOS is a RTOS based on Microkernel architecture considering cost efficient targets like ARM Cortex-M series microcontroller.
 *  TachyOS is composed of three distinct parts (Kernel, AAL and HAL) and depend on each other. but they are agnostic to implementation
 *  of each other by put interface for each core part.
 *
 * ## Kernel
 * > The kernel is composed of core functionalities which can be used from user application or HAL components as well.
 * > Kernel depends on the architecture abstraction layer(AAL) which defines how kernel interact with under lying core part of cpu.
 * > Some kernel components are optional and you can easily switch them off with kernel configuration which can be performed by simple
 * > command line utility. kernel can provide optional system power management and time functionality which depends on a few HAL
 * > components.
 *
 * ## AAL
 * > The AAL(Architecture Abstract Layer) is an abstraction of how the kernel interacts to underlying hardware.
 * > It is designed to maximize portability and fit to various CPU architecture by keeping the total count of interfaces
 * > low so anyone can port TachyOS into a CPU architecture with just small piece of souce code (hopefully under 300 sloc)
 * > As stated earlier, few assumption is applied in AAL design along the way of abstraction. Most significant are listed
 * > below.
 * >
 * > - Hardware support unpriviledged(User) / priviledged(Kernel) mode
 * > - Hardware support separate stack pointer for each mode
 * > - unpriviledged mode can aquire priviledged mode by raise interrupt
 *
 *
 * \section support Supported Target
 *  - STM32F407 (ARM Cortex-M4)
 *  - STM32F417 (ARM Cortex-M4)
 *  - STM32F2XX (ARM Cortex-M3)
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


/*!
 *  \addtogroup core_api Core API
 *  @{
 */

#define tchWaitForever                          0xFFFFFFFF                                               ///< wait forever timeout value
#define tch_assert(api,b,err)                   if(!b){api->Thread->exit(api->Thread->self(),err);}
#define DECLARE_THREADROUTINE(fn)               int fn(const tch_core_api_t* ctx)



/*!
 * \brief Core API for Thread
 */
struct tch_thread_api {

	/*!\fn tch_threadId tch_thread_api::create(thread_config_t* cfg, void* arg)
	 * \brief create thread without putting on ready queue
	 * \param[in] cfg thread configuration, can not be null
	 * \param[in] arg thread arguement, can be null arguement can be accessed by calling getArg()
	 * \return thread id, if create operation is successful, otherwise, it returns NULL
	 * \sa tch_thread_api::start
	 */
	tch_threadId (*create)(thread_config_t* cfg,void* arg);

	/*!
	 *  \fn tchStatus tch_thread_api::start(tch_threadId thread)
	 *  \brief start or put new thread into ready queue depends on its priority
	 *  \param[in] thread id of thread to be started
	 *  \returns tchOk start successfully, otherwise return tchErrorParameter
	 *  \sa tchStatus
	 */
	tchStatus (*start)(tch_threadId thread);

	/*!
	 *  \fn tch_threadId tch_thread_api::self()
	 *  \brief get current thread ID
	 *  \return current thread ID
	 */
	tch_threadId (*self)();

	/*!
	 *  \fn tchStatus tch_thread_api::yield(uint32_t millisec)
	 *  \brief yield CPU runtime to next thread and rescheduled after given time
	 *  \param[in] millisec reschedule time in millisecond, should be determined time
	 *  \return tchOk yield successfully, return tchErrorParameter if tchWaitForever is used as parameter
	 *  \sa tchOK
	 *  \sa tchWaitForever
	 */
	tchStatus (*yield)(uint32_t millisec);
	tchStatus (*sleep)(uint32_t sec);
	tchStatus (*join)(tch_threadId thread,uint32_t timeout);
	tchStatus (*exit)(tch_threadId thread,tchStatus res);
	void (*initConfig)(thread_config_t* cfg,
					tch_thread_routine entry,
					tch_threadPrior prior,
					uint32_t stksz,
					uint32_t heapsz,
					const char* name);
	void* (*getArg)();
};


struct tch_mutex_api {
	tch_mtxId (*create)();
	tchStatus (*lock)(tch_mtxId mtx,uint32_t timeout);
	tchStatus (*unlock)(tch_mtxId mtx);
	tchStatus (*destroy)(tch_mtxId mtx);
};


struct tch_semaphore_api {
	tch_semId (*create)(uint32_t count);
	tchStatus (*lock)(tch_semId sid,uint32_t timeout);
	tchStatus (*unlock)(tch_semId sid);
	tchStatus (*destroy)(tch_semId sid);
};


struct tch_barrier_api {
	tch_barId (*create)(uint8_t thread_cnt);
	tchStatus (*enter)(tch_barId bar,uint32_t timeout);
	tchStatus (*destroy)(tch_barId bar);
};



struct tch_time_api {
	tchStatus (*getWorldTime)(time_t* tp);								//< return GMT time
	tchStatus (*setWorldTime)(time_t epoch_gmt);						//< set GMT time for system
	alrm_Id (*setAlarm)(time_t epoch_gmt,alrmIntv period);				//< set alarm in GMT time
	tchStatus (*waitAlarm)(alrm_Id alrm);
	tchStatus (*cancelAlarm)(alrm_Id alrm);								//< cancel alarm
	tch_timezone (*setTimezone)(const tch_timezone tz);					//< set standard time zone
	tch_timezone (*getTimezone)();										//< get standard time zone
	uint64_t (*getCurrentTimeMills)();									//< get system-wise time in mills (Not guaranteed time)
	uint64_t (*uptimeMills)();											//< get elapsed time in millisec after system init
	time_t (*fromBrokenTime)(struct tm* tp);
	void (*fromEpochTime)(const time_t time, struct tm* dest_tm,tch_timezone tz);
};


/*!
 * \brief posix-like condition variable API struct
 */
struct tch_condvar_api {

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



struct tch_mempool_api {
	tch_mpoolId (*create)(size_t sz,uint32_t plen);
	void* (*alloc)(tch_mpoolId mpool);
	void* (*calloc)(tch_mpoolId mpool);
	tchStatus (*free)(tch_mpoolId mpool,void* block);
	tchStatus (*destroy)(tch_mpoolId mpool);
};

struct tch_event_api {
	tch_eventId (*create)(void);
	int32_t (*set)(tch_eventId ev,int32_t signals);
	int32_t (*clear)(tch_eventId ev,int32_t signals);
	tchStatus (*wait)(tch_eventId ev,int32_t signals,uint32_t millisec);
	tchStatus (*destroy)(tch_eventId ev);
};



struct tch_messageQ_api {
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

	tchStatus (*reset)(tch_msgqId);

	/*!
	 * \brief destroy msg queue
	 */
	tchStatus (*destroy)(tch_msgqId);
};


typedef uint16_t tch_waitqPolicy;

#define WAITQ_POL_THREADPRIORITY	((tch_waitqPolicy) 1)
#define WAITQ_POL_FIFO				((tch_waitqPolicy) 2)
#define WAITQ_POL_LIFO				((tch_waitqPolicy) 3)

struct tch_waitq_api {
	tch_waitqId (*create)(tch_waitqPolicy policy);
	tchStatus (*sleep)(tch_waitqId waitq,uint32_t timeout);
	tchStatus (*wake)(tch_waitqId waitq);
	tchStatus (*wakeAll)(tch_waitqId waitq);
	tchStatus (*destroy)(tch_waitqId waitq);
};


struct tch_mailQ_api {
	tch_mailqId (*create)(uint32_t sz,uint32_t qlen);
	void* (*alloc)(tch_mailqId qid,uint32_t timeout,tchStatus* result);
	tchStatus (*put)(tch_mailqId qid,void* mail,uint32_t timeout);
	tchEvent (*get)(tch_mailqId qid,uint32_t timeout);
	tchStatus (*free)(tch_mailqId qid,void* mail);
	tchStatus (*destroy)(tch_mailqId qid);
};

struct tch_malloc_api {
	void* (*alloc)(size_t size);
	void (*free)(void*);
	void (*mstat)(mstat* statp);
};


struct tch_module_api {
	void* (*request)(int);
	BOOL (*chkdep)(module_map_t* service);
};


#define print_dbg( ...)			 ctx->Dbg->print(0, 0,  __VA_ARGS__)
#define print_warn(...)			 ctx->Dbg->print(1, 0,  __VA_ARGS__)
#define print_error(err, ...)	 ctx->Dbg->print(2, err,__VA_ARGS__)
typedef uint8_t dbg_level;
struct tcn_dbg_api {
	const dbg_level Normal;
	const dbg_level Warn;
	const dbg_level Error;
	void (*print)(dbg_level level,tchStatus code, const char* format, ...);
};


extern DECLARE_THREADROUTINE(main);

/**
 * @} */

#ifdef __cplusplus
}
#endif
#endif /* TCH_H_ */
