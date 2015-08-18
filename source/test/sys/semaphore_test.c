/*
 * semaphore_test.c
 *
 *  Created on: 2014. 8. 20.
 *      Author: innocentevil
 */


#include "semaphore_test.h"

#define TEST_CNT  ((uint8_t) 200)


static DECLARE_THREADROUTINE(child1Routine);
static DECLARE_THREADROUTINE(child2Routine);
static DECLARE_THREADROUTINE(child3Routine);
static void race(const tch* api);


static tch_threadId ch1Id;
static tch_threadId ch2Id;
static tch_threadId ch3Id;

static volatile BOOL spin;

static tch_semId ts;
static uint16_t shVar;

tchStatus semaphore_performTest(tch* api){

	shVar = 0;
	spin = TRUE;


	tch_threadCfg thcfg;
	api->Thread->initCfg(&thcfg,child1Routine,Normal,512 , 0, "child1");
	ch1Id = api->Thread->create(&thcfg,api);


	api->Thread->initCfg(&thcfg,child2Routine,Normal,512, 0 ,"child2");
	ch2Id = api->Thread->create(&thcfg,api);

	api->Thread->initCfg(&thcfg,child3Routine,Normal,512,0, "child3");
	ch3Id = api->Thread->create(&thcfg,api);

	ts = api->Sem->create(1);
	if(api->Sem->lock(ts,tchWaitForever) != tchOK)
		return tchErrorOS;
	api->Thread->start(ch1Id);
	api->Thread->yield(200);
	if(api->Thread->join(ch3Id,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(api->Thread->join(ch2Id,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(api->Thread->join(ch1Id,tchWaitForever) != tchOK)
		return tchErrorOS;

	return tchOK;
}



static DECLARE_THREADROUTINE(child1Routine){
	if(env->Sem->lock(ts,10) != tchErrorTimeoutResource)
		return tchErrorOS;
	env->Sem->unlock(ts);
	if(env->Sem->lock(ts,tchWaitForever) != tchOK)
		return tchErrorOS;
	env->Sem->unlock(ts);
	env->Thread->start(ch2Id);
	env->Thread->start(ch3Id);
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(env);
	env->Thread->yield(5);
	env->Sem->destroy(ts);
	spin = FALSE;

	return tchOK;
}

static DECLARE_THREADROUTINE(child2Routine){
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(env);
	while(spin) env->Thread->yield(0);
	if(env->Sem->lock(ts,tchWaitForever) != tchErrorResource)
		return tchErrorOS;
	return tchOK;
}

static DECLARE_THREADROUTINE(child3Routine){
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(env);
	return tchOK;
}

void race(const tch* api){
	tchStatus res = tchOK;
	if((res = api->Sem->lock(ts,tchWaitForever)) != tchOK)
		return;
	shVar++;
	api->Thread->yield(0);
	api->Sem->unlock(ts);
}
