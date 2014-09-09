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
static void race(tch* api);


static tch_threadId ch1Id;
static tch_threadId ch2Id;
static tch_threadId ch3Id;

static volatile BOOL spin;

static tch_semDef tsd;
static tch_sem_id ts;
static uint16_t shVar;

tchStatus semaphore_performTest(tch* api){

	shVar = 0;
	spin = TRUE;
	uint32_t* th1Stk = api->Mem->alloc(512);
	uint32_t* th2Stk = api->Mem->alloc(512);
	uint32_t* th3Stk = api->Mem->alloc(512);


	tch_thread_cfg thcfg;
	thcfg._t_name = "child1";
	thcfg._t_routine = child1Routine;
	thcfg._t_stack = th1Stk;
	thcfg.t_stackSize = 512;
	thcfg.t_proior = Normal;
	ch1Id = api->Thread->create(&thcfg,api);

	thcfg._t_name = "child2";
	thcfg._t_routine = child2Routine;
	thcfg._t_stack = th2Stk;
	ch2Id = api->Thread->create(&thcfg,api);

	thcfg._t_name = "child3";
	thcfg._t_routine = child3Routine;
	thcfg._t_stack = th3Stk;
	ch3Id = api->Thread->create(&thcfg,api);

	ts = api->Sem->create(&tsd,1);
	if(api->Sem->lock(ts,osWaitForever) != osOK)
		return osErrorOS;
	api->Thread->start(ch1Id);
	api->Thread->sleep(200);
	if(api->Thread->join(ch3Id,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(ch2Id,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(ch1Id,osWaitForever) != osOK)
		return osErrorOS;

	api->Mem->free(th1Stk);
	api->Mem->free(th2Stk);
	api->Mem->free(th3Stk);


	return osOK;
}



static DECLARE_THREADROUTINE(child1Routine){
	if(sys->Sem->lock(ts,10) != osErrorTimeoutResource)
		return osErrorOS;
	sys->Sem->unlock(ts);
	if(sys->Sem->lock(ts,osWaitForever) != osOK)
		return osErrorOS;
	sys->Sem->unlock(ts);
	sys->Thread->start(ch2Id);
	sys->Thread->start(ch3Id);
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(sys);
	sys->Thread->sleep(5);
	sys->Sem->destroy(ts);
	spin = FALSE;

	return osOK;
}

static DECLARE_THREADROUTINE(child2Routine){
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(sys);
	while(spin) sys->Thread->sleep(0);
	if(sys->Sem->lock(ts,osWaitForever) != osErrorResource)
		return osErrorOS;
	return osOK;
}

static DECLARE_THREADROUTINE(child3Routine){
	uint8_t cnt = 0;
	for(;cnt < TEST_CNT;cnt++)
		race(sys);
	return osOK;
}

void race(tch* api){
	tchStatus res = osOK;
	if((res = api->Sem->lock(ts,osWaitForever)) != osOK)
		return;
	shVar++;
	api->Thread->sleep(0);
	api->Sem->unlock(ts);
}
