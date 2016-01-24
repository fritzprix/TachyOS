/*
 * tch_err.h
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#ifndef TCH_ERR_H_
#define TCH_ERR_H_

#include "kernel/util/time.h"
#include "kernel/tch_ktypes.h"

#define KERNEL_PANIC(msg)	tch_kernel_onPanic(__FILE__,__LINE__,msg)

extern void tch_kernel_onSoftException(tch_threadId who,tchStatus err,const char* msg);			//managable error which is cuased by thread level erroneous behavior
extern void tch_kernel_onPanic(const char* floc,int errorno, const char* msg);								//error which can not be handled by kernel (typically caused by kernel bug or boot environments)


#endif /* TCH_ERR_H_ */
