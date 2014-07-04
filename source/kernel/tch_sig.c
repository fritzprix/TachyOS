/*
 * tch_sig.c
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_thread.h"

static int32_t tch_signal_set(tch_thread_id thread,int32_t signals);
static int32_t tch_signal_clear(tch_thread_id thread,int32_t signals);
static osStatus tch_signal_wait(int32_t signals,uint32_t millisec);




__attribute__((section(".data"))) static tch_signal_ix Signa_StaticInstance = {
		tch_signal_set,
		tch_signal_clear,
		tch_signal_wait
};




int32_t tch_signal_set(tch_thread_id thread,int32_t signals){
	if(!(tch_kernelThreadIntegrityCheck(thread) && (signals < (1 << (osFeature_Signals + 1))))){
		return 0x80000000;
	}
	int32_t sig = ((tch_thread_header*) thread)->t_sig.signal;

}

int32_t tch_signal_clear(tch_thread_id thread,int32_t signals){
	tch_thread_header* th_p = (tch_thread_header*) thread;
	int32_t sig = th_p->t_sig.signal;

	return sig;
}

osStatus tch_signal_wait(int32_t signals,uint32_t millisec){
	tch_thread_header* th_p = tch_schedGetRunningThread();

	if((th_p->t_sig.match_target = signals) == th_p->t_sig.signal){
		return osOK;
	}else{
	}
}
