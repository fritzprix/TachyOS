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
#include "tch_mem.h"
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
static void tch_threadInitCfg(tch_threadCfg* cfg);
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


tchStatus tchk_threadIsValid(tch_threadId thread){
	if(!thread)
		return tchErrorParameter;
	if(getThreadHeader(thread)->t_chks != ((uint32_t) THREAD_CHK_PATTERN)){
		getThreadHeader(thread)->t_reent._errno = tchErrorStackOverflow;
		return tchErrorStackOverflow;
	}
	if(getThreadHeader(thread)->t_kthread->t_flag & THREAD_DEATH_BIT)
		return getThreadHeader(thread)->t_reent._errno;
	return tchOK;
}

BOOL tchk_threadIsPrivilidged(tch_threadId thread){
	return ((getThreadKHeader(thread)->t_flag & THREAD_PRIV_BIT) > 0);
}



void tchk_threadInvalidate(tch_threadId thread,tchStatus reason){
	getThreadHeader(thread)->t_reent._errno = reason;
	getThreadHeader(thread)->t_kthread->t_flag |= THREAD_DEATH_BIT;
}


BOOL tchk_threadIsRoot(tch_threadId thread){
	return (getThreadHeader(thread)->t_kthread->t_flag & THREAD_ROOT_BIT);
}


void tchk_threadSetPriority(tch_threadId tid,tch_thread_prior nprior){
	if(nprior > Unpreemtible)
		return;
	getThreadKHeader(tid)->t_prior = nprior;
}

tch_thread_prior tchk_threadGetPriority(tch_threadId tid){
	return getThreadKHeader(tid)->t_prior;
}
/**
 * create new thread
 */
tch_threadId tchk_threadCreateThread(tch_threadCfg* cfg,void* arg,BOOL isroot,BOOL ispriv){
	// allocate kernel thread header from kernel heap
	tch_thread_kheader* kthread = (tch_thread_kheader*) tchk_kernelHeapAlloc(sizeof(tch_thread_kheader));
	memset(kthread,0,sizeof(tch_thread_kheader));
	if(isroot){														// if new thread will be the root thread of a process, parent will be self
		kthread->t_parent = kthread;
		cdsl_dlistPutTail((cdsl_dlistNode_t*) &procList,(cdsl_dlistNode_t*) &kthread->t_siblingLn);		// added in process list
		if(cfg->t_memDef.heap_sz < TCH_CFG_HEAP_MIN_SIZE)			// guarantee minimum heap size
			cfg->t_memDef.heap_sz = TCH_CFG_HEAP_MIN_SIZE;
	}else if(tch_currentThread){									// new thread will be child of caller thread
		kthread->t_parent = tch_currentThread->t_kthread->t_parent;
		cfg->t_memDef.heap_sz = 0;
		cdsl_dlistPutTail(&kthread->t_parent->t_childLn,&kthread->t_siblingLn);
	}else {
		KERNEL_PANIC("tch_thread.c","Null Running Thread");
	}
	if(cfg->t_memDef.stk_sz < TCH_CFG_THREAD_STACK_MIN_SIZE)		// guarantee minimum stack size
		cfg->t_memDef.stk_sz = TCH_CFG_THREAD_STACK_MIN_SIZE;

	if(tchk_userMemInit(kthread,&cfg->t_memDef,isroot) != tchOK)	// prepare memory space of new thread
		KERNEL_PANIC("tch_thread.c","Can't create proccess memory space");

	kthread->t_ctx = tch_port_makeInitialContext(kthread->t_uthread,kthread->t_proc,__tch_thread_entry);
	kthread->t_flag |= isroot? THREAD_ROOT_BIT : 0;
	kthread->t_flag |= ispriv? THREAD_PRIV_BIT : 0;

	cdsl_dlistInit(&kthread->t_palc);
	cdsl_dlistInit(&kthread->t_pshalc);
	cdsl_dlistInit(&kthread->t_upshalc);

	kthread->t_tslot = TCH_ROUNDROBIN_TIMESLOT;
	kthread->t_state = PENDED;
	kthread->t_lckCnt = 0;
	kthread->t_prior = cfg->t_priority;
	kthread->t_to = 0;
	if(!kthread->t_pgId)
		KERNEL_PANIC("tch_thread.c","Can't create proccess memory space");

	kthread->t_uthread->t_arg = arg;														// initialize user level thread header
	kthread->t_uthread->t_fn = cfg->t_routine;
	kthread->t_uthread->t_name = cfg->t_name;
	kthread->t_uthread->t_kRet = tchOK;
#ifdef __NEWLIB__																			// optional part of initialization for reentrant structure required by std libc
	_REENT_INIT_PTR(&kthread->t_uthread->t_reent)
#endif
	kthread->t_uthread->t_chks = THREAD_CHK_PATTERN;
	return (tch_threadId) kthread->t_uthread;
}


extern tchStatus tchk_threadLoadProgram(tch_threadId root,uint8_t* pgm_img,size_t img_sz,uint32_t pgm_entry_offset){
	if(!root || !tchk_threadIsValid(root))
		return tchErrorParameter;
	tch_thread_kheader* root_kheader = ((tch_thread_uheader*) root)->t_kthread;
	memcpy(root_kheader->t_proc,pgm_img,img_sz);
	return tchOK;
}


static tch_threadId tch_threadCreate(tch_threadCfg* cfg,void* arg){
	uint8_t tm = 0;
	if(tch_port_isISR())
		return NULL;
	if(cfg->t_memDef.stk_sz < TCH_CFG_THREAD_STACK_MIN_SIZE)
		cfg->t_memDef.stk_sz = TCH_CFG_THREAD_STACK_MIN_SIZE;

	cfg->t_memDef.heap_sz = 0;
	cfg->t_memDef.pimg_sz = 0;
	uint32_t msz = ((cfg->t_memDef.stk_sz + sizeof(tch_thread_uheader)) | 0xF) + 1;
	if(!cfg->t_memDef.u_mem){
		cfg->t_memDef.u_mem = tch_rti->Mem->alloc(msz);
		cfg->t_memDef.u_memsz = msz;
	}else{
		/**
		 * test read access.
		 * if memory access is not allowed by this thread, memory fault will be raiesd by hardware
		 */
		tm = *cfg->t_memDef.u_mem;
	}
	return (tch_threadId) tch_port_enterSv(SV_THREAD_CREATE,(uword_t) cfg, (uword_t) arg);
}





static tchStatus tch_threadStart(tch_threadId thread){
	if(tch_port_isISR()){                 // check current execution mode (Thread or Handler)
		tchk_schedThreadReady(thread);    // if handler mode call, put current thread in ready queue
		return tchOK;
	}else{
		return tch_port_enterSv(SV_THREAD_START,(uint32_t)thread,0);
	}
}


static tchStatus tch_threadTerminate(tch_threadId thread,tchStatus err){
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_DESTROY,(uint32_t) thread,err);
	}
}


static tch_threadId tch_threadSelf(){
	return (tch_threadId) tch_currentThread;
}


static tchStatus tch_threadSleep(uint32_t sec){
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_SLEEP,sec,0);
	}
}


static tchStatus tch_threadYield(uint32_t millisec){
	if(tch_port_isISR()){
		return tchErrorISR;
	}else{
		return tch_port_enterSv(SV_THREAD_YIELD,millisec,0);
	}
}

static tchStatus tch_threadJoin(tch_threadId thread,uint32_t timeout){
	if(tch_port_isISR()){
		return tchErrorISR;					// unreachable code
	}else{
		return tch_port_enterSv(SV_THREAD_JOIN,(uint32_t) thread,timeout);
	}
}

static void tch_threadInitCfg(tch_threadCfg* cfg){
	memset(cfg,0,sizeof(tch_threadCfg));
}


static void* tch_threadGetArg(){
	return getThreadHeader(tch_currentThread)->t_arg;
}

__attribute__((naked)) static void __tch_thread_entry(tch_thread_uheader* thr_p,tchStatus status){

#ifdef MFEATURE_HFLOAT
	float _force_fctx = 0.1f;
	_force_fctx += 0.1f;
#endif
	tchStatus res = thr_p->t_fn(tch_rti);
	tch_port_enterSv(SV_THREAD_DESTROY,(uint32_t) thr_p,status);
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
	tch_thread_kheader* th_p = getThreadKHeader(thread);
	tch_thread_kheader* ch_p = NULL;

	tchk_userMemFreeAll(th_p);
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
	}
	tch_port_enterSv(SV_THREAD_TERMINATE,(uint32_t) thread,status);
}

