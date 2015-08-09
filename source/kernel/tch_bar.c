/*
 * tch_bar.c
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */




/*!\brief Barrier Implementation
 *
 *
 */
#include "tch.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_bar.h"
#include "kernel/tch_sched.h"
#include "kernel/tch_err.h"


#define TCH_BARRIER_CLASS_KEY        ((uhword_t) 0x2D03)

#define BAR_VALIDATE(bar)			do {\
	((tch_barCb*) bar)->status |= (((uint32_t) bar ^ TCH_BARRIER_CLASS_KEY) & 0xFFFF);\
}while(0)

#define BAR_INVALIDATE(bar)			do {\
	((tch_barCb*) bar)->status &= ~0xFFFF;\
}while(0)

#define BAR_ISVALID(bar)			((((tch_barCb*) bar)->status & 0xFFFF) == (((uint32_t) bar ^ TCH_BARRIER_CLASS_KEY) & 0xFFFF))




static tch_barId tch_bar_create();
static tchStatus tch_bar_wait(tch_barId bar,uint32_t timeout);
static tchStatus tch_bar_signal(tch_barId bar,tchStatus result);
static tchStatus tch_bar_destroy(tch_barId bar);



__attribute__((section(".data"))) static tch_bar_ix Barrier_StaticInstance = {
		tch_bar_create,
		tch_bar_wait,
		tch_bar_signal,
		tch_bar_destroy
};

const tch_bar_ix* Barrier = &Barrier_StaticInstance;



DECLARE_SYSCALL_0(bar_create,tch_barId);
DECLARE_SYSCALL_2(bar_wait,tch_barId,uint32_t,tchStatus);
DECLARE_SYSCALL_2(bar_signal,tch_barId,tchStatus,tchStatus);
DECLARE_SYSCALL_1(bar_destroy,tch_barId,tchStatus);


DEFINE_SYSCALL_0(bar_create,tch_barId) {
	tch_barCb* bar = (tch_barCb*) kmalloc(sizeof(tch_barCb));
	if(!bar)
		KERNEL_PANIC("tch_bar.c","can't allocate barrier");
	tchk_barrierInit(bar,FALSE);
	return (tch_barId) bar;
}

DEFINE_SYSCALL_2(bar_wait,tch_barId,bar,uint32_t,timeout,tchStatus) {
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	tch_barCb* _bar = (tch_barCb*) bar;
	return tchk_schedThreadSuspend((tch_thread_queue*) &_bar->wq,timeout);
}

DEFINE_SYSCALL_2(bar_signal,tch_barId,barId,tchStatus,result,tchStatus){
	tch_barCb* bar = (tch_barCb*) barId;
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	if(cdsl_dlistIsEmpty(&bar->wq))
		return tchOK;
	if(tch_port_isISR()){
		tchk_schedThreadResumeM((tch_thread_queue*)&bar->wq,SCHED_THREAD_ALL,tchOK,TRUE);
		return tchOK;
	}
	return tch_port_enterSv(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,tchOK,0);
}

DEFINE_SYSCALL_1(bar_destroy,tch_barId,barId,tchStatus){
	if((!barId) || (!BAR_ISVALID(barId)))
		return tchErrorParameter;
	tch_barCb* bar = (tch_barCb*) barId;
	BAR_INVALIDATE(barId);
	tchk_schedThreadResumeM((tch_thread_queue*) &bar->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	kfree(bar);
	return tchOK;
}

tch_barId tchk_barrierInit(tch_barCb* bar,BOOL is_static){
	if(!is_static){
		bar = (tch_barCb*) kmalloc(sizeof(tch_barCb));
	}
	memset(bar, 0, sizeof(tch_barCb));
	BAR_VALIDATE(bar);
	cdsl_dlistInit(&bar->wq);
	bar->__obj.__destr_fn =  is_static? (tch_kobjDestr) __tch_noop_destr : (tch_kobjDestr) tch_bar_destroy;
	return bar;
}

tchStatus tchk_barrierDeinit(tch_barCb* bar){
	if((!bar) || (!BAR_ISVALID(bar)))
		return tchErrorParameter;
	BAR_INVALIDATE(bar);
	tchk_schedThreadResumeM((tch_thread_queue*) &bar->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	kfree(bar);
	return tchOK;
}


static tch_barId tch_bar_create(){
	if(tch_port_isISR())
		return NULL;

	return __SYSCALL_0(bar_create);
}

static tchStatus tch_bar_wait(tch_barId bar,uint32_t timeout){
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_2(bar_wait,bar,timeout);
}

static tchStatus tch_bar_signal(tch_barId barId,tchStatus result){
	tch_barCb* bar = (tch_barCb*) barId;
	if(!bar)
		return tchErrorParameter;
	if(tch_port_isISR()){

		if(!BAR_ISVALID(bar))
			return tchErrorParameter;

		tchk_schedThreadResumeM((tch_thread_queue*)&bar->wq,SCHED_THREAD_ALL,tchOK,TRUE);
		return tchOK;
	}
	return __SYSCALL_2(bar_signal,barId,result);
}

static tchStatus tch_bar_destroy(tch_barId barId){
	if(!barId)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;

	return __SYSCALL_1(bar_destroy,barId);
}

