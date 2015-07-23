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



#if defined(__cplusplus)
extern "C" {
#endif


typedef struct _tch_mtx_cb_t  {
	tch_kobj            __obj;
	uint32_t            status;
	tch_thread_queue    que;
	tch_threadId        own;
	tch_threadPrior    svdPrior;
}tch_mtxCb;


/***
 *  mutex  types
 */

extern tch_mtxId tchk_mutexInit(tch_mtxCb* mcb,BOOL is_static);
extern tchStatus tchk_mutexLock(tch_mtxId mtx,uint32_t timeout);
extern tchStatus tchk_mutexUnlock(tch_mtxId mtx);
extern tchStatus tchk_mutexDestroy(tch_mtxId mtx);

#if defined(__cplusplus)
}
#endif


#endif /* TCH_MTX_H_ */
