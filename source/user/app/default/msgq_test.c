/*
 * msgq_test.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 6.
 *      Author: innocentevil
 */


#include "tch.h"
#include "utest.h"

static DECLARE_THREADROUTINE(sender);
static tch_threadId sender_id;

static DECLARE_THREADROUTINE(receiver);
static tch_threadId receiver_id;


static tch_msgqId mid;
static tch_barId mBar;

static tch_gpioHandle* out;
static tch_gpioHandle* in;
tch_core_api_t* Api;

static int irqcnt;
static int usrcnt;
static int miscnt;


tchStatus do_test_msgq(const tch_core_api_t* api){
	Api = (tch_core_api_t*) api;
	mBar = api->Barrier->create();
	irqcnt = 0;
	miscnt = 0;
	usrcnt = 0;


	const tch_messageQ_api_t* MsgQ = api->MsgQ;
	mid = MsgQ->create(10);


	const tch_thread_api_t* Thread = api->Thread;
	thread_config_t tcfg;
	api->Thread->initConfig(&tcfg,sender,Normal,1 << 9,0,"sender");
	sender_id = Thread->create(&tcfg,api);

	api->Thread->initConfig(&tcfg,receiver,Normal,1 << 9,0, "receiver");
	receiver_id = Thread->create(&tcfg,api);

	Thread->start(receiver_id);
	Thread->start(sender_id);


	tch_assert(api,Thread->join(receiver_id,tchWaitForever) == tchOK,tchErrorOS);
	tch_assert(api,Thread->join(sender_id,tchWaitForever) == tchOK,tchErrorOS);
	api->Barrier->destroy(mBar);


	in->unregisterIoEvent(in,1 << 0);

	out->close(out);
	in->close(in);

	return tchOK;

}


static DECLARE_THREADROUTINE(sender){
	uint32_t cnt = 0;
	while(cnt < 50){
		ctx->MsgQ->put(mid,0xFF,tchWaitForever);
		out->out(out,1 << 2,bClear);
		ctx->Thread->yield(1);
		out->out(out,1 << 2,bSet);
		ctx->Thread->yield(1);
		cnt++;
	}
	ctx->Thread->yield(10);
	ctx->MsgQ->destroy(mid);
	return tchOK;
}

static DECLARE_THREADROUTINE(receiver){
	uint32_t cnt = 0;
	tchEvent evt;
	uint32_t mval = 0;
	uint32_t totalMsgcnt = 0;
	while(cnt < 100){
		evt = ctx->MsgQ->get(mid,tchWaitForever);
		totalMsgcnt++;
		if(evt.status == tchEventMessage){
			cnt++;
			if(evt.value.v == 0xFF){
				usrcnt++;
			}else if(evt.value.v == 0xF0){
				irqcnt++;
			}else{
				mval = evt.value.v;
				miscnt++;
			}
		}
	}
	evt = ctx->MsgQ->get(mid,10);
	if(evt.status != tchErrorTimeoutResource)
		return tchErrorOS;
	evt = ctx->MsgQ->get(mid,tchWaitForever);
	if(evt.status != tchErrorResource)
		return tchErrorResource;
	return tchOK;
}


