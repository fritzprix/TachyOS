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
	tch_uobj            __obj;
	uint32_t            state;
	tch_thread_queue    que;
	tch_threadId        own;
	tch_thread_prior    svdPrior;
}tch_mtxCb;


/***
 *  mutex  types
 */

extern void tch_mtxInit(tch_mtxCb* mtx);

#if defined(__cplusplus)
}
#endif


#endif /* TCH_MTX_H_ */
