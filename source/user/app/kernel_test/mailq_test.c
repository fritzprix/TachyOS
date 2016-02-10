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

tchStatus mailq_performTest(tch_core_api_t* api){

	mstat init_mstat,fin_mstat;

	kmstat(&init_mstat);

	testmailq_id = api->MailQ->create(sizeof(person),10);

	const tch_kernel_service_thread* Thread = api->Thread;
	thread_config_t tcfg;
	Thread->initConfig(&tcfg,sender,Normal,1<<9,0,"sender");
	sender_id = Thread->create(&tcfg,api);

	Thread->initConfig(&tcfg,receiver,Normal,1 << 9, 0, "receiver");
	receiver_id = Thread->create(&tcfg,api);

	Thread->start(receiver_id);
	Thread->start(sender_id);
	tchStatus result = api->Thread->join(receiver_id,tchWaitForever);

	api->MailQ->destroy(testmailq_id);

	if(result != tchOK)
		return result;

	kmstat(&fin_mstat);

	if(init_mstat.used != fin_mstat.used)
		result = tchErrorMemoryLeaked;
	return result;
}

DECLARE_THREADROUTINE(sender){
	uint32_t cnt = 0;
	person* p = NULL;
	while(cnt < 100){
		p = ctx->MailQ->alloc(testmailq_id,tchWaitForever,NULL);
		mset(p,0,sizeof(person));
		p->age = 0;
		p->sex = 1;
		ctx->MailQ->put(testmailq_id,p,1000);
		cnt++;
	}
	return tchOK;
}

DECLARE_THREADROUTINE(receiver){
	uint32_t cnt = 0;
	tchEvent evt;
	while(cnt < 100){
		evt = ctx->MailQ->get(testmailq_id,tchWaitForever);
		ctx->MailQ->free(testmailq_id,(void*)evt.value.v);
		if(evt.status == tchEventMail)
			cnt++;
	}
	return tchOK;
}
