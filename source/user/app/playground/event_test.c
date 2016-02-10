/*
 * event_test.c
 *
 *  Created on: 2016. 2. 6.
 *      Author: innocentevil_win
 */




#include "tch.h"
#include "event_test.h"

static DECLARE_THREADROUTINE(waiter);
static DECLARE_THREADROUTINE(waker);



tchStatus do_eventTest(const tch_core_api_t* api)
{
	thread_config_t cfg;

	tch_eventId ev = api->Event->create();
	api->Thread->initConfig(&cfg, waiter, Normal, 0, 0 ,"Water");
	tch_threadId waiterID = api->Thread->create(&cfg,ev);

	api->Thread->initConfig(&cfg, waker, Normal, 0 ,0, "Waker");
	tch_threadId wakerID = api->Thread->create(&cfg,ev);

	api->Thread->start(waiterID);
	api->Thread->start(wakerID);

//	api->Thread->join(waiterID,tchWaitForever);
//	api->Thread->join(wakerID,tchWaitForever);
	return tchOK;
}



static DECLARE_THREADROUTINE(waiter)
{
	tch_eventId ev = ctx->Thread->getArg();
	tchStatus res;
	uint32_t testCnt = 1000;
	tchEvent evt;
	while(testCnt--)
	{
		if((res = ctx->Event->wait(ev, 1, tchWaitForever)) != tchOK)
		{
			return res;
		}
		ctx->Event->clear(ev, 1);
	}
	return tchOK;
}


static DECLARE_THREADROUTINE(waker)
{
	tch_eventId ev = ctx->Thread->getArg();
	tchStatus res;
	uint32_t testCnt = 1000;
	while(testCnt--)
	{
		ctx->Event->set(ev, 1);
		ctx->Thread->yield(0);
	}
	return tchOK;
}

