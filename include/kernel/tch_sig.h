/*
 * tch_sig.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */


#ifndef _TCH_SIG_H
#define _TCH_SIG_H

#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint32_t sig_update_t;

extern void tch_signalInit(tch_signal* sig);
extern int32_t tch_signal_kupdate(tch_threadId thread,int32_t sig);
extern int32_t tch_signal_kwait(tch_threadId thread, int32_t signal,uint32_t timeout);


#if defined(__cplusplus)
}
#endif

#endif
