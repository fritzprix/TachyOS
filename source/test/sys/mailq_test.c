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
static tch_threadId sender_id;

static DECLARE_THREADROUTINE(receiver);
static tch_threadId receiver_id;


tch_mailqId testmailq_id;

tchStatus mailq_performTest(tch* api){
	testmailq_id = api->MailQ->create(sizeof(person),10);

	const tch_thread_ix* Thread = api->Thread;
	tch_threadCfg tcfg;
	tcfg._t_name = "sender";
	tcfg._t_routine = sender;
	tcfg.t_proior = Normal;
	tcfg.t_stackSize = 1 << 9;
	sender_id = Thread->create(&tcfg,api);

	tcfg._t_name = "receiver";
	tcfg._t_routine = receiver;
	receiver_id = Thread->create(&tcfg,api);

	Thread->start(receiver_id);
	Thread->start(sender_id);
	tchStatus result = api->Thread->join(receiver_id,osWaitForever);

	api->MailQ->destroy(testmailq_id);

	return result;
}

DECLARE_THREADROUTINE(sender){
	uint32_t cnt = 0;
	person* p = NULL;
	while(cnt < 100){
		p = env->MailQ->calloc(testmailq_id,osWaitForever,NULL);
		p->age = 0;
		p->sex = 1;
		env->MailQ->put(testmailq_id,p);
		cnt++;
	}
	return osOK;
}

DECLARE_THREADROUTINE(receiver){
	uint32_t cnt = 0;
	osEvent evt;
	while(cnt < 100){
		evt = env->MailQ->get(testmailq_id,osWaitForever);
		env->MailQ->free(testmailq_id,(void*)evt.value.v);
		if(evt.status == osEventMail)
			cnt++;
	}
	return osOK;
}
