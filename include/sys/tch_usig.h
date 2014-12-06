/*
 * tch_usig.h
 *
 *  Created on: 2014. 11. 29.
 *      Author: innocentevil
 */

#ifndef TCH_USIG_H_
#define TCH_USIG_H_

#include "tch_kernel.h"

#if defined(__cplusplus)
extern "C"{
#endif

typedef struct tch_usig_arg_t {
	int      signum;
	void*    sigarg;
}tch_uSiganlArg;

extern void tch_usigInit(tch_usigHandle* sig);

extern tch_sigFuncPtr tch_uSignalSetK(int sig,tch_sigFuncPtr fn);
extern int tch_uSignalRaiseK(tch_threadId thread,tch_uSiganlArg* sarg);
extern BOOL tch_uSignalJmpToHanlder(tch_threadId thread);


#if defined(__cplusplus)
}
#endif

#endif /* TCH_USIG_H_ */
