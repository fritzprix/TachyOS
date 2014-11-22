/*
 * sig_test.c
 *
 *  Created on: 2014. 11. 22.
 *      Author: innocentevil
 */

#include "sig_test.h"



static DECLARE_THREADROUTINE(siggenThread1);
static DECLARE_THREADROUTINE(siggenThread2);




tchStatus signal_performTest(tch* api){
	tch_threadCfg thcfg;
	thcfg._t_name = "siggen1";
	thcfg._t_routine = siggenThread1;
	thcfg.t_heapSize = 0;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	tch_threadId siggne1Id = api->Thread->create(&thcfg,NULL);

}


static DECLARE_THREADROUTINE(siggenThread1){

}

static DECLARE_THREADROUTINE(siggenThread2){

}
