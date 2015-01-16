/*
 * tch_sig->c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_sig.h"
#include "tch_sched.h"



#define SIG_UPDATE_CLR         ((sig_update_t) 1)
#define SIG_UPDATE_SET         ((sig_update_t) 2)

typedef struct tch_signal_arg_s {
	int32_t      sig;
	sig_update_t type;
}tch_signal_arg_t;

static int32_t tch_signal_set(tch_threadId thread,int32_t signals);
static int32_t tch_signal_clear(tch_threadId thread,int32_t signals);
static tchStatus tch_signal_wait(int32_t signals,uint32_t millisec);



__attribute__((section(".data"))) static tch_signal_ix Signal_StaticInstance = {
		tch_signal_set,
		tch_signal_clear,
		tch_signal_wait
};

const tch_signal_ix* Sig = &Signal_StaticInstance;



void tch_signalInit(tch_signal* sig){
	sig->sig_comb = sig->signal = 0;
	tch_listInit(&sig->sig_wq);
}

int32_t tch_signal_kupdate(tch_threadId thread,int32_t sig){
	tch_thread_header* th_p = (tch_thread_header*) thread;
	tch_signal_arg_t* sig_arg = (tch_signal_arg_t*)sig;
	int32_t psig = th_p->t_sig.signal;
	switch(sig_arg->type){
	case SIG_UPDATE_SET:
		th_p->t_sig.signal |= sig_arg->sig;
		break;
	case SIG_UPDATE_CLR:
		th_p->t_sig.signal &= ~sig_arg->sig;
		break;
	}
	if((th_p->t_sig.sig_comb == th_p->t_sig.signal) && (!tch_listIsEmpty(&th_p->t_sig.sig_wq))){
		tch_schedResumeM(&th_p->t_sig.sig_wq,1,th_p->t_sig.signal,TRUE);
	}
	return psig;
}

int32_t tch_signal_kwait(tch_threadId thread, int32_t signal,uint32_t timeout){
	tch_thread_header* th_p = (tch_thread_header*) thread;
	th_p->t_sig.sig_comb = signal;
	if(th_p->t_sig.sig_comb != th_p->t_sig.signal){
		tch_schedSuspendThread(&th_p->t_sig.sig_wq,timeout);
	}
	return th_p->t_sig.signal;
}


int32_t tch_signal_set(tch_threadId thread,int32_t signals){
	tch_signal_arg_t arg;
	if(!tch_threadIsValid(thread))
		return 0x80000000;
	arg.sig = signals;
	arg.type = SIG_UPDATE_SET;
	if(tch_port_isISR())
		return tch_signal_kupdate(thread,&arg);
	else
		return tch_port_enterSv(SV_SIG_UPDATE,(uint32_t) thread,&arg);
}

int32_t tch_signal_clear(tch_threadId thread,int32_t signals){
	tch_signal_arg_t arg;
	if(!tch_threadIsValid(thread))
		return 0x80000000;
	arg.sig = signals;
	arg.type = SIG_UPDATE_CLR;
	if(tch_port_isISR())
		return tch_signal_kupdate(thread,&arg);
	else
		return tch_port_enterSv(SV_SIG_UPDATE,(uint32_t) thread,&arg);
}

tchStatus tch_signal_wait(int32_t signals,uint32_t millisec){
	if(tch_port_isISR()){
		return tchErrorISR;
	}
	return tch_port_enterSv(SV_SIG_WAIT,signals,millisec);

}
