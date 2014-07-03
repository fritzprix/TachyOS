/*
 * tch_sig.c
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"

static int32_t tch_signal_set(tch_thread_id thread,int32_t signals);
static int32_t tch_signal_clear(tch_thread_id thread,int32_t signals);
static int32_t tch_signal_wait(int32_t signals,uint32_t millisec);




__attribute__((section(".data"))) static tch_signal_ix Signa_StaticInstance = {
		tch_signal_set,
		tch_signal_clear,
		tch_signal_wait
};




int32_t tch_signal_set(tch_thread_id thread,int32_t signals){

}

int32_t tch_signal_clear(tch_thread_id thread,int32_t signals){

}

int32_t tch_signal_wait(int32_t signals,uint32_t millisec){

}
