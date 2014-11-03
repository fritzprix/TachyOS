/*
 * mtx_test.c
 *
 *  Created on: 2014. 8. 17.
 *      Author: manics99
 */


#include "tch.h"
#include "mtx_test.h"


static DECLARE_THREADROUTINE(child1Routine);
static DECLARE_THREADROUTINE(child2Routine);

static tch_threadId child1;
static tch_threadId child2;

static void race(tch* api);
static tch_mtxId mmtx;


/*!
 * \brief Mtx Unit Test
 */
tchStatus mtx_performTest(tch* api){


	uint32_t* ch1Stack = api->Mem->alloc(512 * sizeof(uint8_t));
	uint32_t* ch2Stack = api->Mem->alloc(512 * sizeof(uint8_t));

	tch_thread_ix* Thread = api->Thread;
	tch_threadCfg thCfg;
	thCfg._t_name = "child1_mtx";
	thCfg._t_routine = child1Routine;
	thCfg._t_stack = ch1Stack;
	thCfg.t_proior = Normal;
	thCfg.t_stackSize = 512;
	child1 = Thread->create(&thCfg,api);

	thCfg._t_name = "child2_mtx";
	thCfg._t_routine = child2Routine;
	thCfg._t_stack = ch2Stack;
	thCfg.t_stackSize = 512;
	thCfg.t_proior = Normal;
	child2 = Thread->create(&thCfg,api);

	mmtx = api->Mtx->create();
	if(api->Mtx->lock(mmtx,osWaitForever) != osOK)
		return osErrorOS;
	api->Thread->start(child1);
	api->Thread->sleep(200);
	api->Mtx->unlock(mmtx);


	tchStatus result = api->Thread->join(child1,osWaitForever);

	api->Mem->free(ch1Stack);
	api->Mem->free(ch2Stack);
	return result;

}


static void race(tch* api){
	uint32_t idx = 0;
	tch_mtx_ix* Mtx = api->Mtx;
	for(idx = 0;idx < 100;idx++){
		if(Mtx->lock(mmtx,osWaitForever) == osOK){
			api->Thread->sleep(0);
			Mtx->unlock(mmtx);
		}
	}
}

static DECLARE_THREADROUTINE(child1Routine){
	tchStatus result = osOK;
	if((result = env->Mtx->lock(mmtx,10)) != osErrorTimeoutResource)    // Timeout expected
		return osErrorOS;
	if((result = env->Mtx->unlock(mmtx)) != osErrorResource)
		return osErrorOS;
	if((result = env->Mtx->lock(mmtx,osWaitForever)) != osOK)
		return osErrorOS;
	env->Thread->start(child2);
	race(env);
	env->Mtx->destroy(mmtx);
	return env->Thread->join(child2,osWaitForever);
}

static DECLARE_THREADROUTINE(child2Routine){
	race(env);
	tchStatus result = osOK;
	env->Thread->sleep(50);
	if((result = env->Mtx->lock(mmtx,osWaitForever)) == osErrorResource)
		return osOK;
	return osErrorOS;
}
