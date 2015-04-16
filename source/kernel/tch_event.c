/*
 * tch_event.c
 *
 *  Created on: 2015. 1. 20.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_event.h"


#define SIG_UPDATE_CLR         ((sig_update_t) 1)
#define SIG_UPDATE_SET         ((sig_update_t) 2)

#define EVENT_CLASS_KEY                        ((uint16_t )0xDABC)

#define EVENT_VALIDATE(ins)                 do {\
	((tch_eventCb*) ins)->status = (((uint32_t) ins ^ EVENT_CLASS_KEY) & 0xFFFF);\
}while(0)

#define EVENT_INVALIDATE(ins)               do {\
	((tch_eventCb*) ins)->status &= ~0xFFFF;\
}while(0)

#define EVENT_ISVALID(ins)    ((((tch_eventCb*) ins)->status & 0xFFFF) == (((uint32_t) ins ^ EVENT_CLASS_KEY) & 0xFFFF))

typedef uint8_t     sig_update_t;

struct tch_eventCb {
	tch_uobj            __obj;
	uint32_t              status;
	int32_t               ev_msk;
	int32_t               ev_signal;
	tch_thread_queue      ev_blockq;
};


typedef struct tch_event_sig_arg_s {
	int32_t      ev_signal;
	sig_update_t type;
}tch_event_sarg_t;

typedef struct tch_event_wait_arg_s {
	int32_t      ev_sigmsk;
	uint32_t     timeout;
}tch_event_warg_t;

static tch_eventId tch_eventCreate();
static int32_t tch_eventSet(tch_eventId ev,int32_t signals);
static int32_t tch_eventClear(tch_eventId ev,int32_t signals);
static tchStatus tch_eventWait(tch_eventId ev,int32_t signal_msk,uint32_t millisec);
static tchStatus tch_eventDestroy(tch_eventId ev);


__attribute__((section(".data"))) static tch_event_ix Event_StaticInstance = {
		tch_eventCreate,
		tch_eventSet,
		tch_eventClear,
		tch_eventWait,
		tch_eventDestroy
};


const tch_event_ix* Event = &Event_StaticInstance;



static tch_eventId tch_eventCreate(){
	tch_eventCb* evcb = (tch_eventCb*) tch_shMemAlloc(sizeof(tch_eventCb),TRUE);
	tch_eventCb initcb;
	uStdLib->string->memset(&initcb,0,sizeof(tch_eventCb));
	initcb.__obj.destructor = (tch_uobjDestr) tch_eventDestroy;
	tch_listInit((tch_lnode*)&initcb.ev_blockq);
	return (tch_eventId) tch_port_enterSv(SV_EV_INIT,(uword_t) evcb,(uword_t) &initcb);
}

tch_eventId tchk_eventInit(tch_eventCb* evcb,tch_eventCb* initcb){
	uStdLib->string->memcpy(evcb,initcb,sizeof(tch_eventCb));
	EVENT_VALIDATE(evcb);
	return (tch_eventId) evcb;
}


static int32_t tch_eventSet(tch_eventId ev,int32_t signals){
	tch_event_sarg_t arg;
	if(!ev || !EVENT_ISVALID(ev))
		return 0;
	arg.ev_signal = signals;
	arg.type = SIG_UPDATE_SET;
	if(tch_port_isISR())
		return tchk_eventUpdate(ev,&arg);
	else
		return tch_port_enterSv(SV_EV_UPDATE,(uint32_t) ev,(uint32_t)&arg);
}

static int32_t tch_eventClear(tch_eventId ev,int32_t signals){
	tch_event_sarg_t arg;
	if(!ev || !EVENT_ISVALID(ev))
		return 0;
	arg.ev_signal= signals;
	arg.type = SIG_UPDATE_CLR;
	if(tch_port_isISR())
		return tchk_eventUpdate(ev,&arg);
	else
		return tch_port_enterSv(SV_EV_UPDATE,(uint32_t) ev,(uint32_t) &arg);
}

static tchStatus tch_eventWait(tch_eventId ev,int32_t signal_msk,uint32_t millisec){
	tch_event_warg_t warg;
	if(!ev || !EVENT_ISVALID(ev))
		return tchErrorParameter;
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	warg.timeout = millisec;
	warg.ev_sigmsk = signal_msk;
	return tch_port_enterSv(SV_EV_WAIT,(uint32_t) ev,(uint32_t)&warg);
}

static tchStatus tch_eventDestroy(tch_eventId ev){
	if(!ev)
		return tchErrorParameter;
	if(!EVENT_ISVALID(ev))
		return tchErrorParameter;
	if(!tch_port_isISR()){
		return tchErrorISR;
	}
	tch_port_enterSv(SV_EV_DEINIT,(uint32_t) ev,0);
	tch_shMemFree(ev);
	return tchOK;
}

tchStatus tchk_eventWait(tch_eventId id,void* arg){
	tch_event_warg_t* warg = (tch_event_warg_t*) arg;
	tch_eventCb* evcb = (tch_eventCb*) id;
	evcb->ev_msk = warg->ev_sigmsk;
	if((evcb->ev_msk & evcb->ev_signal) != evcb->ev_msk){
		tchk_schedThreadSuspend(&evcb->ev_blockq,warg->timeout);
	}
	return tchOK;
}


int32_t tchk_eventUpdate(tch_eventId id,void* arg){
	tch_event_sarg_t* sarg = (tch_event_sarg_t*) arg;
	tch_eventCb* evcb = (tch_eventCb*) id;
	int32_t psig = evcb->ev_signal;
	switch(sarg->type){
	case SIG_UPDATE_SET:
		evcb->ev_signal |= sarg->ev_signal;
		break;
	case SIG_UPDATE_CLR:
		evcb->ev_signal &= ~sarg->ev_signal;
		break;
	}
	if(((evcb->ev_msk & evcb->ev_signal) == evcb->ev_msk) && (!tch_listIsEmpty(&evcb->ev_blockq))){
		tchk_schedThreadResumeM(&evcb->ev_blockq,1,tchOK,TRUE);
	}
	return psig;
}

void tchk_eventDeinit(tch_eventId id){
	tch_eventCb* evcb = (tch_eventCb*) id;
	if(!tch_listIsEmpty(&evcb->ev_blockq)){
		tchk_schedThreadResumeM(&evcb->ev_blockq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	}
}


