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
#include "tch_thread.h"
#include "tch_sched.h"





static int32_t tch_signal_set(tch_threadId thread,int32_t signals);
static int32_t tch_signal_clear(tch_threadId thread,int32_t signals);
static tchStatus tch_signal_wait(int32_t signals,uint32_t millisec);



__attribute__((section(".data"))) static tch_signal_ix Signal_StaticInstance = {
		tch_signal_set,
		tch_signal_clear,
		tch_signal_wait
};

const tch_signal_ix* Sig = &Signal_StaticInstance;





int32_t tch_signal_set(tch_threadId thread,int32_t signals){
	if((signals >= (1 << (osFeature_Signals + 1))) || !Thread->isValid(thread)){
		return 0x80000000;
	}
	tch_thread_header* th_p = (tch_thread_header*) thread;
	int32_t sig = th_p->t_sig->signal;
	th_p->t_sig->signal |= signals;
	if((th_p->t_sig->sig_comb == th_p->t_sig->signal) && (!tch_listIsEmpty(&th_p->t_sig->sig_wq))){
		if(tch_port_isISR())
			tch_port_enterSvFromIsr(SV_SIG_MATCH,(uint32_t) thread,0);
		else
			tch_port_enterSvFromUsr(SV_SIG_MATCH,(uint32_t) thread,0);
	}
	return sig;


}

int32_t tch_signal_clear(tch_threadId thread,int32_t signals){
	if((signals >= (1 << (osFeature_Signals + 1))) || !Thread->isValid(thread)){
		return 0x80000000;
	}
	tch_thread_header* th_p = (tch_thread_header*) thread;
	int32_t sig = th_p->t_sig->signal;
	th_p->t_sig->signal &= ~signals;
	if((th_p->t_sig->sig_comb == th_p->t_sig->signal) && (!tch_listIsEmpty(&th_p->t_sig->sig_wq))){
		if(tch_port_isISR())
			tch_port_enterSvFromIsr(SV_SIG_MATCH,(uint32_t)thread,0);
		else
			tch_port_enterSvFromUsr(SV_SIG_MATCH,(uint32_t)thread,0);

	}
	return sig;
}

tchStatus tch_signal_wait(int32_t signals,uint32_t millisec){
	tch_thread_header* th_p = tch_schedGetRunningThread();
	if(!signals > (1 << (osFeature_Signals + 1))){
		return osErrorParameter;
	}
	if(tch_port_isISR()){
		return osErrorISR;
	}
	if((th_p->t_sig->sig_comb = signals) == th_p->t_sig->signal){
		return osOK;
	}else{
		return tch_port_enterSvFromUsr(SV_SIG_WAIT,signals,millisec);
	}
}
