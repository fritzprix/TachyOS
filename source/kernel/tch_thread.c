/*
 * tch_thread.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 17.
 *      Author: innocentevil
 */

#include "tch_err.h"
#include "tch_kernel.h"
#include "tch_thread.h"
#include "tch_kmalloc.h"
#include "tch_mm.h"
#include "tch_segment.h"
#include <sys/reent.h>

#define THREAD_CHK_PATTERN		((uint32_t) 0xF3F3D5D5)
#define getThreadHeader(th_id)  ((tch_thread_uheader*) th_id)

#define THREAD_ROOT_BIT			((uint8_t) 1 << 0)
#define THREAD_DEATH_BIT		((uint8_t) 1 << 1)
#define THREAD_PRIV_BIT			((uint8_t) 1 << 2)


/**
 *  public accessible function
 */
/***
 *  create new thread
 *   - inflate thread header structure onto the very top of thread stack area
 *   - initialize thread header from thread configuration type
 *   - inflate thread context for start (R4~LR)
 *     -> LR point Entry Routine of Thread
 *
 */
__USER_API__ static tch_threadId tch_thread_create(thread_config_t* cfg,void* arg);
__USER_API__ static tchStatus tch_thread_start(tch_threadId thread);
__USER_API__ static tch_threadId tch_thread_self();
__USER_API__ static tchStatus tch_thread_sleep(uint32_t sec);
__USER_API__ static tchStatus tch_thread_yield(uint32_t millisec);
__USER_API__ tchStatus tch_thread_exit(tch_threadId thread,tchStatus result);
__USER_API__ static tchStatus tch_thread_join(tch_threadId thread,uint32_t timeout);
__USER_API__ static void tch_thread_initConfig(thread_config_t* cfg,
											tch_thread_routine entry,
											tch_threadPrior prior,
											uint32_t req_stksz,
											uint32_t req_heapsz,
											const char* name);
__USER_API__ static void* tch_thread_getArg();

static void __tch_thread_entry(tch_thread_uheader* thr_p,tchStatus status) __attribute__((naked));
static void tch_thread_validate(tch_threadId thread);
static void tch_thread_invalidate(tch_threadId thread,tchStatus reason);


__USER_RODATA__ tch_kernel_service_thread Thread_IX = {
		.create = tch_thread_create,
		.start = tch_thread_start,
		.self = tch_thread_self,
		.yield = tch_thread_yield,
		.sleep = tch_thread_sleep,
		.exit = tch_thread_exit,
		.join = tch_thread_join,
		.initConfig = tch_thread_initConfig,
		.getArg = tch_thread_getArg,
};


__USER_RODATA__ const tch_kernel_service_thread* Thread = &Thread_IX;


DECLARE_SYSCALL_2(thread_create,thread_config_t*,void*,tch_threadId);
DECLARE_SYSCALL_1(thread_start,tch_threadId,tchStatus);
DECLARE_SYSCALL_2(thread_terminate,tch_threadId,tchStatus,tchStatus);
DECLARE_SYSCALL_2(thread_exit,tch_threadId,tchStatus,tchStatus);
DECLARE_SYSCALL_1(thread_sleep,uint32_t,tchStatus);
DECLARE_SYSCALL_1(thread_yield,uint32_t,tchStatus);
DECLARE_SYSCALL_2(thread_join,tch_threadId,uint32_t,tchStatus);




DEFINE_SYSCALL_2(thread_create,thread_config_t*,cfg,void*,arg,tch_threadId){
	if(!cfg)
		return NULL;
	return tch_threadCreateThread(cfg,arg,FALSE,FALSE,NULL);
}

DEFINE_SYSCALL_1(thread_start,tch_threadId,id,tchStatus){
	tch_schedStart(id);
	return tchOK;
}

DEFINE_SYSCALL_2(thread_exit,tch_threadId,tid,tchStatus,err,tchStatus){
	if (!tid)
		return tchErrorParameter;
	if (tch_threadIsValid(tid) != tchOK)
		return tchErrorParameter;

	tch_thread_uheader* thr = (tch_thread_uheader*) tid;
	if (tid == current){
		tch_thread_kheader* kth = getThreadKHeader(thr);
		kth->flag &= ~THREAD_DEATH_BIT;
		kth->prior = Low;
		tch_port_enterPrivThread(__tch_thread_atexit, (uint32_t) tid, err, 0);
	}else{
		tch_thread_invalidate(tid, err);
	}
	return tchOK;
}

DEFINE_SYSCALL_2(thread_terminate,tch_threadId,tid,tchStatus,res,tchStatus){
	if(!tid)
		return tchErrorParameter;
	if(tch_threadIsValid(tid) != tchOK)
		return tchErrorParameter;

	tch_schedTerminate(tid,res);
	tch_thread_kheader* kth = getThreadKHeader(tid);
	cdsl_dlistRemove(&kth->t_siblingLn);			// remove link to sibling list
	tch_mmProcClean(kth);
	kfree(kth);
	return tchOK;
}

DEFINE_SYSCALL_1(thread_sleep,uint32_t,sec_timeout,tchStatus){
	return tch_schedYield(sec_timeout,SECOND,SLEEP);
}

DEFINE_SYSCALL_1(thread_yield,uint32_t,ms_timeout,tchStatus){
	return tch_schedYield(ms_timeout,mSECOND,WAIT);
}

DEFINE_SYSCALL_2(thread_join,tch_threadId,id,uint32_t,timeout,tchStatus){
	tch_thread_kheader* th = ((tch_thread_uheader*) id)->kthread;
	if(th->state != TERMINATED){
		return tch_schedWait((tch_thread_queue*) &th->t_joinQ,timeout);
	}
	return tchOK;
}


tchStatus tch_threadIsValid(tch_threadId thread){
	if(!thread)
		return tchErrorParameter;
	if(getThreadHeader(thread)->chks != ((uint32_t) THREAD_CHK_PATTERN)){
		getThreadHeader(thread)->kRet = tchErrorStackOverflow;
		return tchErrorStackOverflow;
	}
	if(getThreadHeader(thread)->kthread->flag & THREAD_DEATH_BIT)
		return getThreadHeader(thread)->kRet;
	return tchOK;
}

BOOL tch_threadIsPrivilidged(tch_threadId thread){
	return ((getThreadKHeader(thread)->flag & THREAD_PRIV_BIT) > 0);
}

static void tch_thread_validate(tch_threadId thread){
	if(!thread)
		return;
	getThreadHeader(thread)->chks = ((uint32_t) THREAD_CHK_PATTERN);
	getThreadKHeader(thread)->flag = 0;
}


static void tch_thread_invalidate(tch_threadId thread,tchStatus reason){
	getThreadHeader(thread)->kRet = reason;
	getThreadHeader(thread)->kthread->flag |= THREAD_DEATH_BIT;
}


BOOL tch_threadIsRoot(tch_threadId thread){
	return (getThreadHeader(thread)->kthread->flag & THREAD_ROOT_BIT);
}


void tch_threadSetPriority(tch_threadId tid,tch_threadPrior nprior){
	if(nprior > Unpreemtible)
		return;
	getThreadKHeader(tid)->prior = nprior;
}

tch_threadPrior tch_threadGetPriority(tch_threadId tid){
	return getThreadKHeader(tid)->prior;
}
/**
 * create new thread
 */

/**
 * \brief create thread
 * \note kernel mode function (should not be invoked in user mode)
 * \param[in] cfg thread configuration
 */

tch_threadId tch_threadCreateThread(thread_config_t* cfg,void* arg,BOOL isroot,BOOL ispriv,struct proc_header* proc){
	// allocate kernel thread header from kernel heap
	tch_thread_kheader* kthread = (tch_thread_kheader*) kmalloc(sizeof(tch_thread_kheader));
	if(!kthread)
		return NULL;

	mset(kthread,0,(sizeof(tch_thread_kheader)));
	if(isroot){ 			// if new thread is the root thread of a process, parent will be self
		if(!proc){			// if new thread is trusted thread
			proc = &default_prochdr;
			proc->entry = cfg->entry;
			proc->req_stksz = cfg->stksz;
			proc->req_heapsz = cfg->heapsz;
			proc->argv = arg;
			proc->argv_sz = 0;
			proc->flag = PROCTYPE_STATIC | HEADER_ROOT_THREAD;
			kthread->permission = 0xffffffff;
		}
		kthread->parent = kthread;
		if(!tch_mmProcInit(kthread, proc)){
			kfree(kthread);
			return NULL;
		}
		cdsl_dlistPutTail((cdsl_dlistNode_t*) &procList,(cdsl_dlistNode_t*) &kthread->t_siblingLn);		// added in process list
	}else if(current){									// new thread will be child of caller thread
		proc = &default_prochdr;
		proc->entry = cfg->entry;
		proc->req_stksz = cfg->stksz;
		proc->req_heapsz = cfg->heapsz;
		proc->argv = arg;
		proc->argv_sz = 0;
		proc->flag = HEADER_CHILD_THREAD;
		kthread->parent = current->kthread;
		kthread->permission = kthread->parent->permission;		// inherit parent permission
		if(!tch_mmProcInit(kthread, proc)){
			kfree(kthread);
			return NULL;
		}
		cdsl_dlistPutTail(&kthread->parent->child_list,&kthread->t_siblingLn);
	}else {
		KERNEL_PANIC("tch_thread.c","Null Running Thread");
	}
	tch_thread_validate(kthread->uthread);
	kthread->ctx = tch_port_makeInitialContext(kthread->uthread,(void*)((kthread->mm.stk_region->poff + kthread->mm.stk_region->psz) << CONFIG_PAGE_SHIFT),__tch_thread_entry);
	kthread->flag |= isroot? THREAD_ROOT_BIT : 0;
	kthread->flag |= ispriv? THREAD_PRIV_BIT : 0;

	kthread->tslot = CONFIG_ROUNDROBIN_TIMESLOT;
	kthread->state = PENDED;
	kthread->lckCnt = 0;
	kthread->prior = cfg->priority;
	kthread->to = 0;
	kthread->uthread->name = cfg->name;
	mcpy(&kthread->uthread->ctx,tch_rti,sizeof(tch));
	return (tch_threadId) kthread->uthread;
}


__USER_API__ static tch_threadId tch_thread_create(thread_config_t* cfg,void* arg){
	if(!cfg)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_threadId) __SYSCALL_2(thread_create,cfg,arg);
}



__USER_API__ static tchStatus tch_thread_start(tch_threadId thread){
	if(tch_port_isISR()){			// check current execution mode (Thread or Handler)
		tch_schedReady(thread);		// if handler mode call, put current thread in ready queue
		return tchOK;
	}
	return __SYSCALL_1(thread_start,thread);
}

__USER_API__ tchStatus tch_thread_exit(tch_threadId thread,tchStatus result){
	if (tch_port_isISR()) {
		return __thread_exit(thread, result);
	}
	return __SYSCALL_2(thread_exit, thread,result);
}


__USER_API__ static tch_threadId tch_thread_self(){
	return (tch_threadId) current;
}


__USER_API__ static tchStatus tch_thread_sleep(uint32_t sec){
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(thread_sleep,sec);
}


__USER_API__ static tchStatus tch_thread_yield(uint32_t millisec){
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(thread_yield,millisec);
}

__USER_API__ static tchStatus tch_thread_join(tch_threadId thread,uint32_t timeout){
	if(tch_port_isISR())
		return tchErrorISR;					// unreachable code
	return __SYSCALL_3(thread_join,thread,timeout,0);
}


__USER_API__ static void tch_thread_initConfig(thread_config_t* cfg,
							  tch_thread_routine entry,
							  tch_threadPrior prior,
							  uint32_t req_stksz,
							  uint32_t req_heapsz,
							  const char* name){
	mset(cfg,0,sizeof(thread_config_t));
	cfg->heapsz = req_heapsz;
	cfg->stksz = req_stksz;
	cfg->priority = prior;
	cfg->name = name;
	cfg->entry = entry;
}


__USER_API__ static void* tch_thread_getArg(){
	return current->t_arg;
}

__attribute__((naked)) static void __tch_thread_entry(tch_thread_uheader* thr_p,tchStatus status){

#ifdef MFEATURE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif
	tchStatus res = thr_p->fn(&thr_p->ctx);
	__SYSCALL_2(thread_exit,thr_p,res);
}


/**
 *  thread clean-up point called just before thread is terminated.
 *  - clean up allocated resources from user heap
 *  - clean up allocated resources from shareable heap
 *  - if it has child thread, all the child-threads of its own will be terminated
 *
 *
 *  note : this function should be called from the privilidged access context
 */
__attribute__((naked)) void __tch_thread_atexit(tch_threadId thread,int status){
	if(!thread)
		KERNEL_PANIC("tch_thread.c","invalid tid at atexit");
	tch_thread_kheader* th_p = getThreadKHeader(thread);
	tch_thread_kheader* ch_p = NULL;


	while(!cdsl_dlistIsEmpty(&th_p->child_list)){												// if has child, kill all of them
		ch_p = (tch_thread_kheader*) cdsl_dlistDequeue((cdsl_dlistNode_t*) &th_p->child_list);
		ch_p = container_of(ch_p, tch_thread_kheader,t_siblingLn);
		tch_thread_exit(ch_p->uthread,status);
		Thread->join(ch_p->uthread,tchWaitForever);
	}

	tch_destroyAllKobjects();	// destruct kobject
	tch_shmCleanup(thread);		// clean up memory
	__SYSCALL_2(thread_terminate,thread,status);
}

