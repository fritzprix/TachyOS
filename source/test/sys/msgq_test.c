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
static DECLARE_THREADSTACK(sender_stack,1 << 8);
static tch_thread_id sender_id;

static DECLARE_THREADROUTINE(receiver);
static DECLARE_THREADSTACK(receiver_stack,1 << 8);
static tch_thread_id receiver_id;

static tch_msgQDef(test,10);

static tch_msgQue_id mid;


tchStatus do_msgqBaseTest(tch* api){

	const tch_msgq_ix* MsgQ = api->MsgQ;
	mid = MsgQ->create(&msgQ_test);

	const tch_thread_ix* Thread = api->Thread;
	tch_thread_cfg tcfg;
	tcfg._t_name = "sender";
	tcfg._t_routine = sender;
	tcfg._t_stack = sender_stack;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 1 << 8;
	sender_id = Thread->create(&tcfg,api);

	tcfg._t_name = "receiver";
	tcfg._t_routine = receiver;
	tcfg._t_stack = receiver_stack;
	receiver_id = Thread->create(&tcfg,api);

	Thread->start(receiver_id);
	Thread->start(sender_id);
	return api->Thread->join(receiver_id,osWaitForever);

}


DECLARE_THREADROUTINE(sender){
	tch* api = (tch*) arg;
	uint32_t cnt = 0;
	while(cnt < 100){
		api->Thread->sleep(20);
		api->MsgQ->put(mid,0xFF,osWaitForever);
		cnt++;
	}
	return osOK;
}

DECLARE_THREADROUTINE(receiver){
	tch* api = (tch*) arg;
	uint32_t cnt = 0;
	while(cnt < 100){
		osEvent evt = api->MsgQ->get(mid,osWaitForever);
		if(evt.status == osOK)
			cnt++;
	}
	return osOK;
}
