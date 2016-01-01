/*
 * thread_test.c
 *
 *  Created on: 2015. 12. 31.
 *      Author: hyunbok
 */


#include "utest.h"

static DECLARE_THREADROUTINE(test_run);

tchStatus do_test_thread(const tch* ctx)
{
	thread_config_t config;
	ctx->Thread->initConfig(&config, test_run,Normal,720,0,"test_thread1");
	tch_threadId child1 = ctx->Thread->create(&config,NULL);
	ctx->Thread->start(child1);
//	ctx->Thread->join(child1,tchWaitForever);
	return tchOK;
}


static DECLARE_THREADROUTINE(test_run)
{
	ctx->Thread->sleep(1);
	return tchOK;
}
