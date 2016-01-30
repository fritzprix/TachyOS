/*
 * thread_test.c
 *
 *  Created on: 2015. 12. 31.
 *      Author: hyunbok
 */


#include "utest.h"

static DECLARE_THREADROUTINE(test_run);

tchStatus do_thread_test(const tch_core_api_t* ctx)
{
	thread_config_t config;
	uint32_t cnt = 0;
	ctx->Thread->initConfig(&config, test_run,Normal,790,0,"test_thread1");
	tch_threadId child1 = ctx->Thread->create(&config,&cnt);
	ctx->Thread->start(child1);

	if(ctx->Thread->join(child1,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(cnt != 100)
		return tchErrorOS;
	return tchOK;
}


static DECLARE_THREADROUTINE(test_run)
{
	uint32_t* cnt = (uint32_t*) ctx->Thread->getArg();
	tch_mtxId mtx = ctx->Mtx->create();
	tch_condvId condv = ctx->Condv->create();
	while(*cnt != 100){
		(*cnt)++;
	}
	return tchOK;
}
