/*
 * tch_mtx.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 30.
 *      Author: innocentevil
 */

#ifndef TCH_MTX_H_
#define TCH_MTX_H_

#include "tch_kobj.h"
#include "tch_lock.h"


#if defined(__cplusplus)
extern "C" {
#endif


typedef struct _tch_mtx_cb_t  {
	tch_kobj            __obj;			///< base object to preventing kernel heap leak unin
	lock_t		__unlockable;	///< unlockable struct for leaving critical section
	uint32_t            status;
	tch_thread_queue    que;
	tch_threadId        own;
	tch_threadPrior     svdPrior;
}tch_mtxCb;


/***
 *  mutex  types
 */
extern tch_mutex_api_t Mutex_IX;
extern tchStatus tch_mutexInit(tch_mtxCb* mcb);
extern tchStatus tch_mutexDeinit(tch_mtxCb* mcb);

#if defined(__cplusplus)
}
#endif


#endif /* TCH_MTX_H_ */
