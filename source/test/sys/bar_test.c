/*
 * bar_test.c
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */



/*! unit test for barrier
 *  -> 3 Thread(s) is initiated and wait in barrier
 *  -> one of the thread will signal main thread before wait in barrier
 *  -> main thread is woke up and yield 50 ms to guarantee all child thread in waiting
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

static tch_barId bid;

tchStatus barrier_performTest(tch* ctx){


	mstat init_mstat,fin_mstat;

	kmstat(&init_mstat);

	tch_threadCfg tcfg;
	ctx->Thread->initCfg(&tcfg,child1Routine,Normal,512, 0 ,"child1");
	child1Id = ctx->Thread->create(&tcfg,ctx);

	ctx->Thread->initCfg(&tcfg,child2Routine,Normal,512,0, "child2");
	child2Id = ctx->Thread->create(&tcfg,ctx);


	ctx->Thread->initCfg(&tcfg,child3Routine,Normal,512,0,"child3");
	child3Id = ctx->Thread->create(&tcfg,ctx);

	tch_assert(ctx,child1Id && child2Id && child3Id,tchErrorOS);

	bid = ctx->Barrier->create();

	ctx->Thread->start(child1Id);
	ctx->Thread->start(child2Id);
	ctx->Thread->start(child3Id);

	ctx->Thread->yield(10);
	ctx->Barrier->signal(bid,tchOK);
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchErrorResource)
		return tchErrorOS;

	if(ctx->Thread->join(child1Id,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Thread->join(child2Id,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Thread->join(child3Id,tchWaitForever) != tchOK)
		return tchErrorOS;

	kmstat(&fin_mstat);
	if(init_mstat.used != fin_mstat.used)
		return tchErrorMemoryLeaked;
	return tchOK;
}


static DECLARE_THREADROUTINE(child1Routine){
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchErrorResource)
		return tchErrorOS;
	return tchOK;
}

static DECLARE_THREADROUTINE(child2Routine){
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchErrorResource)
		return tchErrorOS;
	return tchOK;
}

static DECLARE_THREADROUTINE(child3Routine){
	if(ctx->Barrier->wait(bid,tchWaitForever) != tchOK)
		return tchErrorOS;
	ctx->Thread->yield(20);
	mstat bfor_stat,after_stat;
	ctx->Barrier->destroy(bid);
	return tchOK;
}
