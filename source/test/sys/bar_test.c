/*
 * bar_test.c
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */



/*! unit test for barrier
 *  -> 3 Thread(s) is initiated and wait in barrier
 *  -> one of the thread will signal main thread before wait in barrier
 *  -> main thread is woke up and sleep 50 ms to guarantee all child thread in waiting
 *  -> and unlock barrier ==> all child thread should get 'osOk'
 *
 *
 *
 *
 */

#include "tch.h"
#include "bar_test.h"

static DECLARE_THREADROUTINE(child1Routine);
static DECLARE_THREADROUTINE(child2Routine);
static DECLARE_THREADROUTINE(child3Routine);

static tch_threadId child1Id;
static tch_threadId child2Id;
static tch_threadId child3Id;
static tch_threadId mainId;

static tch_barId bid;

tchStatus barrier_performTest(tch* api){
	mainId = api->Thread->self();


	tch_threadCfg tcfg;
	tcfg._t_name = "child1";
	tcfg._t_routine = child1Routine;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 512;

	child1Id = api->Thread->create(&tcfg,api);

	tcfg._t_name = "child2";
	tcfg._t_routine = child2Routine;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 512;
	child2Id = api->Thread->create(&tcfg,api);


	tcfg._t_name = "child3";
	tcfg._t_routine = child3Routine;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 512;
	child3Id = api->Thread->create(&tcfg,api);

	tch_assert(api,child1Id && child2Id && child3Id,osErrorOS);

	bid = api->Barrier->create();

	api->Thread->start(child1Id);
	api->Thread->start(child2Id);
	api->Thread->start(child3Id);


	api->Sig->wait(1,osWaitForever);
	api->Thread->sleep(10);
	api->Barrier->signal(bid,osOK);
	if(api->Barrier->wait(bid,osWaitForever) != osErrorResource)
		return osErrorOS;

	if(api->Thread->join(child1Id,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(child2Id,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(child3Id,osWaitForever) != osOK)
		return osErrorOS;



	return osOK;
}


static DECLARE_THREADROUTINE(child1Routine){
	if(env->Barrier->wait(bid,osWaitForever) != osOK)
		return osErrorOS;
	if(env->Barrier->wait(bid,osWaitForever) != osErrorResource)
		return osErrorOS;
	return osOK;
}

static DECLARE_THREADROUTINE(child2Routine){
	if(env->Barrier->wait(bid,osWaitForever) != osOK)
		return osErrorOS;
	if(env->Barrier->wait(bid,osWaitForever) != osErrorResource)
		return osErrorOS;
	return osOK;
}

static DECLARE_THREADROUTINE(child3Routine){
	env->Sig->set(mainId,1);
	if(env->Barrier->wait(bid,osWaitForever) != osOK)
		return osErrorOS;
	env->Thread->sleep(20);
	env->Barrier->destroy(bid);
	return osOK;
}
