/*
 * thread_test.c
 *
 *  Created on: 2015. 9. 17.
 *      Author: innocentevil
 */

#include "tch.h"
#include "tch_mm.h"
#include "thread_test.h"

static DECLARE_THREADROUTINE(run);

tchStatus thread_performTest(tch* ctx){

	mstat init_mstat;
	mstat fin_mstat;
	BOOL threadStarted = FALSE;
	tch_threadCfg tcfg;
	tch_threadId child;
	uint8_t cnt = 10;
	tch_barId bar = ctx->Barrier->create();

	kmstat(&init_mstat);

	/**
	 *  Memory Leakage Test
	 *  create & destroy root mode thread
	 */

	while(cnt--){
		threadStarted = FALSE;
		ctx->Thread->initCfg(&tcfg,run,Normal,0,0,"test_run");
		child = (tch_threadId) tch_threadCreateThread(&tcfg,bar,TRUE,TRUE,NULL);
		ctx->Thread->start(child);
		ctx->Barrier->wait(bar,tchWaitForever);
		ctx->Thread->join(child,tchWaitForever);
	}
	kmstat(&fin_mstat);
	if(fin_mstat.used != init_mstat.used)
		return tchErrorOS;



	kmstat(&init_mstat);

	while(cnt--){
		threadStarted = FALSE;
		ctx->Thread->initCfg(&tcfg,run,Normal,0,0,"test_run");
		child = ctx->Thread->create(&tcfg,bar);
		ctx->Thread->start(child);
		ctx->Barrier->wait(bar,tchWaitForever);
		ctx->Thread->join(child,tchWaitForever);
	}
	kmstat(&fin_mstat);
	if(fin_mstat.used != init_mstat.used)
		return tchErrorOS;
	return tchOK;
}

static DECLARE_THREADROUTINE(run) {
	tch_barId bar = ctx->Thread->getArg();
	ctx->Barrier->signal(bar,tchOK);
	tch_mtxId mtx = ctx->Mtx->create();			// leaked mutex
	return tchOK;
}


