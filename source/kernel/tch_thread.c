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
static tch_threadId tch_threadCreate(tch_threadCfg* cfg,void* arg);
static tchStatus tch_threadStart(tch_threadId thread);
static tchStatus tch_threadTerminate(tch_threadId thread,tchStatus err);
static tch_threadId tch_threadSelf();
static tchStatus tch_threadSleep(uint32_t sec);
static tchStatus tch_threadYield(uint32_t millisec);
static tchStatus tch_threadJoin(tch_threadId thread,uint32_t timeout);
static void tch_threadInitCfg(tch_threadCfg* cfg,
							  tch_thread_routine entry,
							  tch_threadPrior prior,
							  uint32_t req_stksz,
							  uint32_t req_heapsz,
							  const char* name);
static void* tch_threadGetArg();
static void __tch_thread_entry(tch_thread_uheader* thr_p,tchStatus status) __attribute__((naked));


__attribute__((section(".data"))) static tch_thread_ix tch_threadix = {
		tch_threadCreate,
		tch_threadStart,
		tch_threadTerminate,
		tch_threadSelf,
		tch_threadYield,
		tch_threadSleep,
		tch_threadJoin,
		tch_threadInitCfg,
		tch_threadGetArg,
};


const tch_thread_ix* Thread = &tch_threadix;


DECLARE_SYSCALL_2(thread_create,tch_threadCfg*,void*,tch_threadId);
DECLARE_SYSCALL_1(thread_start,tch_threadId,tchStatus);
DECLARE_SYSCALL_2(thread_terminate,tch_threadId,tchStatus err,tchStatus);
DECLARE_SYSCALL_1(thread_sleep,uint32_t,tchStatus);
DECLARE_SYSCALL_1(thread_yield,uint32_t,tchStatus);
DECLARE_SYSCALL_2(thread_join,tch_threadId,uint32_t,tchStatus);




DEFINE_SYSCALL_2(thread_create,tch_threadCfg*,cfg,void*,arg,tch_threadId){
	if(!cfg)
		return NULL;
	return tchk_threadCreateThread(cfg,arg,FALSE,FALSE,NULL);
}

DEFINE_SYSCALL_1(thread_start,tch_threadId,id,tchStatus){
	tch_schedStart(id);
	return tchOK;
}

DEFINE_SYSCALL_2(thread_terminate,tch_threadId,id,tchStatus,err,tchStatus){
	tchk_schedTerminate(id,err);
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

tchStatus tchk_threadIsValid(tch_threadId thread){
	if(!thread)
		return tchErrorParameter;
	if(getThreadHeader(thread)->chks != ((uint32_t) THREAD_CHK_PATTERN)){
		getThreadHeader(thread)->reent._errno = tchErrorStackOverflow;
		return tchErrorStackOverflow;
	}
	if(getThreadHeader(thread)->kthread->flag & THREAD_DEATH_BIT)
		return getThreadHeader(thread)->reent._errno;
	return tchOK;
}

BOOL tchk_threadIsPrivilidged(tch_threadId thread){
	return ((getThreadKHeader(thread)->flag & THREAD_PRIV_BIT) > 0);
}



void tchk_threadInvalidate(tch_threadId thread,tchStatus reason){
	getThreadHeader(thread)->reent._errno = reason;
	getThreadHeader(thread)->kthread->flag |= THREAD_DEATH_BIT;
}


BOOL tchk_threadIsRoot(tch_threadId thread){
	return (getThreadHeader(thread)->kthread->flag & THREAD_ROOT_BIT);
}


void tchk_threadSetPriority(tch_threadId tid,tch_threadPrior nprior){
	if(nprior > Unpreemtible)
		return;
	getThreadKHeader(tid)->prior = nprior;
}

tch_threadPrior tchk_threadGetPriority(tch_threadId tid){
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

tch_threadId tchk_threadCreateThread(tch_threadCfg* cfg,void* arg,BOOL isroot,BOOL ispriv,struct proc_header* proc){
	// allocate kernel thread header from kernel heap
	tch_thread_kheader* kthread = (tch_thread_kheader*) kmalloc(sizeof(tch_thread_kheader));
	if(!kthread){
		kfree(kthread);
		return NULL;
	}

	memset(kthread,0,(sizeof(tch_thread_kheader)));

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
	}else if(tch_currentThread){									// new thread will be child of caller thread
		proc = &default_prochdr;
		proc->entry = cfg->entry;
		proc->req_stksz = cfg->stksz;
		proc->req_heapsz = cfg->heapsz;
		proc->argv = arg;
		proc->argv_sz = 0;
		proc->flag = HEADER_CHILD_THREAD;
		kthread->parent = tch_currentThread->kthread;
		kthread->permission = kthread->parent->permission;		// inherit parent permission
		if(!tch_mmProcInit(kthread, proc)){
			kfree(kthread);
			return NULL;
		}
		cdsl_dlistPutTail(&kthread->parent->child_list,&kthread->t_siblingLn);
	}else {
		KERNEL_PANIC("tch_thread.c","Null Running Thread");
	}

	kthread->ctx = tch_port_makeInitialContext(kthread->uthread,(void*)((kthread->mm.stk_region->poff + kthread->mm.stk_region->psz) << CONFIG_PAGE_SHIFT),__tch_thread_entry);
	kthread->flag |= isroot? THREAD_ROOT_BIT : 0;
	kthread->flag |= ispriv? THREAD_PRIV_BIT : 0;

	kthread->tslot = CONFIG_ROUNDROBIN_TIMESLOT;
	kthread->state = PENDED;
	kthread->lckCnt = 0;
	kthread->prior = cfg->priority;
	kthread->to = 0;

#ifdef __NEWLIB__																			// optional part of initialization for reentrant structure required by std libc
	_REENT_INIT_PTR(&kthread->uthread->reent)
#endif
	kthread->uthread->chks = THREAD_CHK_PATTERN;
	kthread->uthread->name = cfg->name;
	return (tch_threadId) kthread->uthread;
}


static tch_threadId tch_threadCreate(tch_threadCfg* cfg,void* arg){
	if(!cfg)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_threadId) __SYSCALL_2(thread_create,cfg,arg);
}



static tchStatus tch_threadStart(tch_threadId thread){
	if(tch_port_isISR()){                 // check current execution mode (Thread or Handler)
		tch_schedReady(thread);    // if handler mode call, put current thread in ready queue
		return tchOK;
	}
	return __SYSCALL_1(thread_start,thread);
}


static tchStatus tch_threadTerminate(tch_threadId thread,tchStatus err){
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	return __SYSCALL_2(thread_terminate,thread,err);
}


static tch_threadId tch_threadSelf(){
	return (tch_threadId) tch_currentThread;
}


static tchStatus tch_threadSleep(uint32_t sec){
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(thread_sleep,sec);
}


static tchStatus tch_threadYield(uint32_t millisec){
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(thread_yield,millisec);
}

static tchStatus tch_threadJoin(tch_threadId thread,uint32_t timeout){
	if(tch_port_isISR())
		return tchErrorISR;					// unreachable code
	return __SYSCALL_3(thread_join,thread,timeout,0);
}


static void tch_threadInitCfg(tch_threadCfg* cfg,
							  tch_thread_routine entry,
							  tch_threadPrior prior,
							  uint32_t req_stksz,
							  uint32_t req_heapsz,
							  const char* name){
	memset(cfg,0,sizeof(tch_threadCfg));
	cfg->heapsz = req_heapsz;
	cfg->stksz = req_stksz;
	cfg->priority = prior;
	cfg->name = name;
	cfg->entry = entry;
}


static void* tch_threadGetArg(){
	return tch_currentThread->t_arg;
}

__attribute__((naked)) static void __tch_thread_entry(tch_thread_uheader* thr_p,tchStatus status){

#ifdef MFEATURE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif
	tchStatus res = thr_p->fn(tch_rti);
	tch_port_enterSv(SV_THREAD_DESTROY,(uint32_t) thr_p,status,0);
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
__attribute__((naked)) void __tchk_thread_atexit(tch_threadId thread,int status){
	if(!thread)
		KERNEL_PANIC("tch_thread.c","invalid tid at atexit");
	tch_thread_kheader* th_p = getThreadKHeader(thread);
	tch_thread_kheader* ch_p = NULL;


	// terminate all the child threads
	while(!cdsl_dlistIsEmpty(&th_p->child_list)){
		ch_p = cdsl_dlistDequeue((cdsl_dlistNode_t*) &th_p->child_list);
		ch_p = container_of(ch_p, tch_thread_kheader,t_siblingLn);
		Thread->terminate(ch_p->uthread,status);
		Thread->join(ch_p->uthread,tchWaitForever);
	}

	// destruct kobject

	// destruct sh object

	// clean up memory
	tch_mmProcClean(th_p);

	/*
	tchk_shareableMemFreeAll(th_p);

	if(th_p->t_flag & THREAD_ROOT_BIT){
		while(!cdsl_dlistIsEmpty(&th_p->t_childLn)){
			ch_p = (tch_thread_kheader*) ((uint32_t) cdsl_dlistDequeue((cdsl_dlistNode_t*) &th_p->t_childLn) - 3 * sizeof(cdsl_dlistNode_t));
			if(ch_p){
				Thread->terminate(ch_p,status);
				Thread->join(ch_p,tchWaitForever);
			}
		}
		tchk_pageRelease(th_p->t_pgId);
		tchk_kernelHeapFree(th_p);
	}else{
		cdsl_dlistRemove(&th_p->t_siblingLn);
		tch_currentThread = th_p->t_parent->t_uthread;
		uMem->free(&th_p->t_uthread->t_destr);
	}*/
	tch_port_enterSv(SV_THREAD_TERMINATE,(uint32_t) thread,status,0);
}

