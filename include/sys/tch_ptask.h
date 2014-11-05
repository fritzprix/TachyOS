/*
 * tch_ptask.h
 * !\ simple task framework based on pop-up thread
 *
 *
 *  Created on: 2014. 11. 4.
 *      Author: innocentevil
 */

#ifndef TCH_PTASK_H_
#define TCH_PTASK_H_

#include "tch_TypeDef.h"

typedef tchStatus (*tch_ptask_fn)(int id,const tch* env,void* arg);
typedef tch_thread_prior tch_ptask_prior;

typedef struct _tch_ptask_ix_t{
	tchStatus (*schedule)(int id,tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout);
}tch_ptask_ix;

const tch_ptask_ix* tch_initpTask(const tch* env);


#endif /* TCH_PTASK_H_ */
