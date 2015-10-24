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



	const tch_kernel_service_thread* Thread = api->Thread;
	thread_config_t thCfg;
	api->Thread->initConfig(&thCfg,child1Routine,Normal,512 , 0, "child1_mtx");
	child1 = Thread->create(&thCfg,api);

	api->Thread->initConfig(&thCfg,child2Routine,Normal,512, 0 , "child2_mtx");
	child2 = Thread->create(&thCfg,api);

	mmtx = api->Mtx->create();
	if(api->Mtx->lock(mmtx,tchWaitForever) != tchOK)
		return tchErrorOS;
	api->Thread->start(child1);
	api->Thread->yield(200);
	api->Mtx->unlock(mmtx);


	tchStatus result = api->Thread->join(child1,tchWaitForever);

	return result;

}


static void race(tch* api){
	uint32_t idx = 0;
	const tch_kernel_service_mtx* Mtx = api->Mtx;
	for(idx = 0;idx < 100;idx++){
		if(Mtx->lock(mmtx,tchWaitForever) == tchOK){
			api->Thread->yield(0);
			Mtx->unlock(mmtx);
		}
	}
}

static DECLARE_THREADROUTINE(child1Routine){
	tchStatus result = tchOK;
	if((result = ctx->Mtx->lock(mmtx,10)) != tchErrorTimeoutResource)    // Timeout expected
		return tchErrorOS;
	if((result = ctx->Mtx->unlock(mmtx)) != tchErrorResource)
		return tchErrorOS;
	if((result = ctx->Mtx->lock(mmtx,tchWaitForever)) != tchOK)
		return tchErrorOS;
	ctx->Thread->start(child2);
	race(ctx);
	ctx->Mtx->destroy(mmtx);
	return ctx->Thread->join(child2,tchWaitForever);
}

static DECLARE_THREADROUTINE(child2Routine){
	race(ctx);
	tchStatus result = tchOK;
	ctx->Thread->yield(50);
	if((result = ctx->Mtx->lock(mmtx,tchWaitForever)) == tchErrorResource)
		return tchOK;
	return tchErrorOS;
}
