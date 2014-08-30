/*
 * mailq_test.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 7.
 *      Author: innocentevil
 */

#include "tch.h"
#include "mailq_test.h"

typedef struct person {
	uint32_t age;
	uint32_t sex;
} person;

static DECLARE_THREADROUTINE(sender);
static DECLARE_THREADSTACK(sender_stack,1 << 8);
static tch_thread_id sender_id;

static DECLARE_THREADROUTINE(receiver);
static DECLARE_THREADSTACK(receiver_stack,1 << 8);
static tch_thread_id receiver_id;


tch_mailQDef(testmail,10,person);
tch_mailQue_id testmailq_id;

tchStatus do_mailQBaseTest(tch* api){
	testmailq_id = api->MailQ->create(tch_access_mailq(testmail));

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
	person* p = NULL;
	while(cnt < 100){
		p = api->MailQ->calloc(testmailq_id,osWaitForever);
		p->age = 0;
		p->sex = 1;
		api->MailQ->put(testmailq_id,p);
		api->Thread->sleep(20);
		cnt++;
	}
	return osOK;
}

DECLARE_THREADROUTINE(receiver){
	tch* api = (tch*) arg;
	uint32_t cnt = 0;
	osEvent evt;
	while(cnt < 100){
		evt = api->MailQ->get(testmailq_id,osWaitForever);
		api->MailQ->free(testmailq_id,(void*)evt.value.v);
		if(evt.status == osEventMail)
			cnt++;
	}
	return osOK;
}
