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



#include "msgq_test.h"

static DECLARE_THREADROUTINE(sender);
static tch_threadId sender_id;

static DECLARE_THREADROUTINE(receiver);
static tch_threadId receiver_id;


static tch_msgQue_id mid;
static tch_barId mBar;


tchStatus msgq_performTest(tch* api){
	mBar = api->Barrier->create();

	uint8_t* sender_stk = (uint8_t*) api->Mem->alloc(1 << 9);
	uint8_t* receiver_stk = (uint8_t*) api->Mem->alloc(1 << 9);

	const tch_msgq_ix* MsgQ = api->MsgQ;
	mid = MsgQ->create(10);

	const tch_thread_ix* Thread = api->Thread;
	tch_thread_cfg tcfg;
	tcfg._t_name = "sender";
	tcfg._t_routine = sender;
	tcfg._t_stack = sender_stk;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 1 << 9;
	sender_id = Thread->create(&tcfg,api);

	tcfg._t_name = "receiver";
	tcfg._t_routine = receiver;
	tcfg._t_stack = receiver_stk;
	receiver_id = Thread->create(&tcfg,api);

	Thread->start(receiver_id);
	Thread->start(sender_id);


	tch_assert(api,Thread->join(receiver_id,osWaitForever) == osOK,osErrorOS);
	tch_assert(api,Thread->join(sender_id,osWaitForever) == osOK,osErrorOS);

	api->Mem->free(sender_stk);
	api->Mem->free(receiver_stk);

	return osOK;

}


DECLARE_THREADROUTINE(sender){
	tch* api = (tch*) arg;
	uint32_t cnt = 0;
	while(cnt < 100){
		api->Thread->sleep(0);
		api->MsgQ->put(mid,0xFF,osWaitForever);
		cnt++;
	}
	api->Barrier->wait(mBar,osWaitForever);
	api->Thread->sleep(10);
	api->MsgQ->destroy(mid);
	return osOK;
}

DECLARE_THREADROUTINE(receiver){
	tch* api = (tch*) arg;
	uint32_t cnt = 0;
	osEvent evt;
	while(cnt < 100){
		evt = api->MsgQ->get(mid,osWaitForever);
		if(evt.status == osEventMessage)
			cnt++;
	}
	evt = api->MsgQ->get(mid,10);
	if(evt.status != osErrorTimeoutResource)
		return osErrorOS;
	api->Barrier->signal(mBar,osOK);
	evt = api->MsgQ->get(mid,osWaitForever);
	if(evt.status != osErrorResource)
		return osErrorResource;
	return osOK;
}
