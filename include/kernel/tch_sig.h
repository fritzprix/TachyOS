/*
 * tch_sig.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */


#ifndef _TCH_SIG_H
#define _TCH_SIG_H

struct _tch_signal_ix_t {
	int32_t (*set)(tch_thread_id thread,int32_t signals);
	int32_t (*clear)(tch_thread_id thread,int32_t signals);
	int32_t (*wait)(int32_t signals,uint32_t millisec);
};

#endif
