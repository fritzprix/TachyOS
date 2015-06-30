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
#include "tch_kernel.h"
#include "tch_bar.h"
#include "tch_sched.h"
#include "tch_mem.h"


#define TCH_BARRIER_CLASS_KEY        ((uhword_t) 0x2D03)

#define BAR_VALIDATE(bar)			do {\
	((tch_barCb*) bar)->status |= (((uint32_t) bar ^ TCH_BARRIER_CLASS_KEY) & 0xFFFF);\
}while(0)

#define BAR_INVALIDATE(bar)			do {\
	((tch_barCb*) bar)->status &= ~0xFFFF;\
}while(0)

#define BAR_ISVALID(bar)			((((tch_barCb*) bar)->status & 0xFFFF) == (((uint32_t) bar ^ TCH_BARRIER_CLASS_KEY) & 0xFFFF))



struct tch_bar_cb_t{
	tch_kobj                 __obj;
	uint32_t                 status;
	cdsl_dlistNode_t         	     wq;
};

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



static tch_barId tch_bar_create(){
	tch_barCb* bar = (tch_barCb*) tch_shMemAlloc(sizeof(tch_barCb),TRUE);
	if(!bar)
		return NULL;
	tch_port_enterSv(SV_BAR_INIT,(uword_t) bar,(uword_t) FALSE);
	return (tch_barId) bar;
}


void tchk_barrierInit(tch_barCb* bar,BOOL is_static){
	memset(bar, 0, sizeof(tch_barCb));
	BAR_VALIDATE(bar);
	cdsl_dlistInit(&bar->wq);
	bar->__obj.__destr_fn =  is_static? (tch_kobjDestr) __tch_noop_destr : (tch_kobjDestr) tch_bar_destroy;
}

tchStatus tchk_barrierDeinit(tch_barCb* bar){
	BAR_INVALIDATE(bar);
	tchk_schedThreadResumeM((tch_thread_queue*) &bar->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	return tchOK;
}

static tchStatus tch_bar_wait(tch_barId bar,uint32_t timeout){
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return tch_port_enterSv(SV_THREAD_SUSPEND,(uint32_t)&((tch_barCb*)bar)->wq,timeout);
}

static tchStatus tch_bar_signal(tch_barId barId,tchStatus result){
	tch_barCb* bar = (tch_barCb*) barId;
	if(!bar || !BAR_ISVALID(bar))
		return tchErrorParameter;
	if(cdsl_dlistIsEmpty(&bar->wq))
		return tchOK;
	if(tch_port_isISR()){
		tchk_schedThreadResumeM((tch_thread_queue*)&bar->wq,SCHED_THREAD_ALL,tchOK,TRUE);
		return tchOK;
	}
	return tch_port_enterSv(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,tchOK);
}

static tchStatus tch_bar_destroy(tch_barId barId){
	tch_barCb* bar = (tch_barCb*) barId;
	tchStatus result = tchOK;
	if((!bar) || (!BAR_ISVALID(bar)))
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	result =  tch_port_enterSv(SV_BAR_DEINIT,(uint32_t)&bar->wq,tchErrorResource);
	tch_shMemFree(bar);
	return result;
}

