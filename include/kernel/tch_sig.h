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

struct _tch_signal_ix_t {
	int32_t (*set)(tch_thread_id thread,int32_t signals);
	int32_t (*clear)(tch_thread_id thread,int32_t signals);
	osStatus (*wait)(int32_t signals,uint32_t millisec);
};

#endif
