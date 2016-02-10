/*
 * msgq_test.c
 *
 *  Created on: 2016. 2. 10.
 *      Author: innocentevil_win
 */




#include "tch.h"
#include "msgq_test.h"


static DECLARE_THREADROUTINE(waiter);
static DECLARE_THREADROUTINE(waker);
const uint32_t MAX_TEST_COUNT = 1000;


tchStatus do_msgqTest(const tch_core_api_t* api)
{
	tch_msgqId mq = api->MsgQ->create(10);
	thread_config_t cfg;
	api->Thread->initConfig(&cfg,waiter, Normal, 0, 0, "waiter");
	tch_threadId waiterId = api->Thread->create(&cfg,mq);


	api->Thread->initConfig(&cfg, waker, Normal, 0, 0, "waker");
	tch_threadId wakerId = api->Thread->create(&cfg,mq);


	api->Thread->start(waiterId);
	api->Thread->start(wakerId);

	api->Thread->join(waiterId,tchWaitForever);
	api->Thread->join(wakerId,tchWaitForever);

	return tchOK;
}


static DECLARE_THREADROUTINE(waiter)
{
	tch_msgqId mq = ctx->Thread->getArg();
	uint32_t tcnt = MAX_TEST_COUNT;
	tchEvent evt;
	while(tcnt--)
	{
		if((evt = ctx->MsgQ->get(mq, tchWaitForever)).status != tchEventMessage)
			return evt.status;
	}
	return tchOK;
}

static DECLARE_THREADROUTINE(waker)
{
	tch_msgqId mq = ctx->Thread->getArg();
	uint32_t tcnt = MAX_TEST_COUNT;
	while(tcnt--)
	{
		if(ctx->MsgQ->put(mq, 1, tchWaitForever) != tchOK)
			return tchErrorOS;
	}
	return tchOK;
}
