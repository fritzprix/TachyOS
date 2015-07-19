/*
 * tch_thread.h
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

#ifndef TCH_THREAD_H_
#define TCH_THREAD_H_


#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif




/**\brief create new thread
 * \note this function should be invoked from privilidged mode.
 * \param[in] cfg thread configuration struct
 * \param[in] arg thread execution arguement
 * \param[in] root whether new thread is root or not
 *
 */
extern tch_threadId tchk_threadCreateThread(tch_threadCfg* cfg,void* arg,BOOL isroot,BOOL ispriv,struct proc_header* proc);
extern tchStatus tchk_threadLoadProgram(tch_threadId root,uint8_t* pgm_img,size_t img_sz,uint32_t pgm_entry_offset);
extern tchStatus tchk_threadIsValid(tch_threadId thread);
extern BOOL tchk_threadIsPrivilidged(tch_threadId thread);
extern void tchk_threadInvalidate(tch_threadId thread,tchStatus reason);
extern BOOL tchk_threadIsRoot(tch_threadId thread);
extern void tchk_threadSetPriority(tch_threadId tid,tch_threadPrior nprior);
extern tch_threadPrior tchk_threadGetPriority(tch_threadId tid);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_THREAD_H_ */
