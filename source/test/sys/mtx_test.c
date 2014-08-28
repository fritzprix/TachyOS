/*
 * mtx_test.c
 *
 *  Created on: 2014. 8. 17.
 *      Author: manics99
 */


#include "tch.h"
#include "mtx_test.h"


DECLARE_THREADSTACK(child1Stack,(1 << 9));
DECLARE_THREADSTACK(child2Stack,(1 << 9));

static DECLARE_THREADROUTINE(child1Routine);
static DECLARE_THREADROUTINE(child2Routine);

tch_thread_id child1;
tch_thread_id child2;

static void race(tch* api);
tch_mtxDef mtxdef;
tch_mtx_id mmtx;


/*!
 * \brief Mtx Unit Test
 */
tchStatus mtx_performTest(tch* api){

	tch_thread_ix* Thread = api->Thread;
	tch_thread_cfg thCfg;
	thCfg._t_name = "child1_mtx";
	thCfg._t_routine = child1Routine;
	thCfg._t_stack = child1Stack;
	thCfg.t_proior = Normal;
	thCfg.t_stackSize = (1 << 9);
	child1 = Thread->create(&thCfg,api);

	thCfg._t_name = "child2_mtx";
	thCfg._t_routine = child2Routine;
	thCfg._t_stack = child2Stack;
	thCfg.t_stackSize = (1 << 9);
	thCfg.t_proior = Normal;
	child2 = Thread->create(&thCfg,api);

	mmtx = api->Mtx->create(&mtxdef);
	if(api->Mtx->lock(mmtx,osWaitForever) != osOK)
		return osErrorOS;
	api->Thread->start(child1);
	api->Thread->sleep(500);
	api->Mtx->unlock(mmtx);




	tchStatus result = api->Thread->join(child1,osWaitForever);
	return result;

}


static void race(tch* api){
	uint32_t idx = 0;
	tch_mtx_ix* Mtx = api->Mtx;
	for(idx = 0;idx < 100;idx++){
		if(Mtx->lock(mmtx,osWaitForever) == osOK){
			api->Thread->sleep(5);
			Mtx->unlock(mmtx);
		}
	}
}

static DECLARE_THREADROUTINE(child1Routine){
	tch* api = (tch*) arg;
	tchStatus result = osOK;
	if((result = api->Mtx->lock(mmtx,10)) != osErrorTimeoutResource)    // Timeout expected
		return osErrorOS;
	if((result = api->Mtx->unlock(mmtx)) != osErrorResource)
		return osErrorOS;
	if((result = api->Mtx->lock(mmtx,osWaitForever)) != osOK)
		return osErrorOS;
	api->Thread->start(child2);
	race(api);
	api->Mtx->destroy(mmtx);
	return api->Thread->join(child2,osWaitForever);
}

static DECLARE_THREADROUTINE(child2Routine){
	tch* api = (tch*) arg;
	race(arg);
	tchStatus result = osOK;
	api->Thread->sleep(50);
	if((result = api->Mtx->lock(mmtx,osWaitForever)) == osErrorResource)
		return osOK;
	return osErrorOS;
}
