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

static DECLARE_IO_CALLBACK(ioEventListener);

static tch_msgQue_id mid;
static tch_barId mBar;

static tch_GpioHandle* out;
static tch_GpioHandle* in;

static volatile tch* Api;
static int irqcnt;
static int usrcnt;
static int miscnt;

tchStatus msgq_performTest(tch* api){
	Api = api;
	mBar = api->Barrier->create();
	irqcnt = 0;
	miscnt = 0;
	usrcnt = 0;

	uint8_t* sender_stk = (uint8_t*) api->Mem->alloc(1 << 9);
	uint8_t* receiver_stk = (uint8_t*) api->Mem->alloc(1 << 9);

	const tch_msgq_ix* MsgQ = api->MsgQ;
	mid = MsgQ->create(10);

	tch_GpioCfg iocfg;
	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.OpenDrain;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	iocfg.Speed = api->Device->gpio->Speed.Mid;
	out = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_0,1 << 2,&iocfg,osWaitForever,ActOnSleep);
	out->out(out,1 << 2,bSet);

	iocfg.Mode = api->Device->gpio->Mode.In;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	iocfg.Speed = api->Device->gpio->Speed.Mid;
	in = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_0,1 << 0,&iocfg,osWaitForever,ActOnSleep);

	tch_GpioEvCfg evcfg;
	api->Device->gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	evcfg.EvCallback = ioEventListener;
	uint32_t pmsk = 1 << 0;
	in->registerIoEvent(in,&evcfg,&pmsk);

	const tch_thread_ix* Thread = api->Thread;
	tch_threadCfg tcfg;
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
	api->Barrier->destroy(mBar);

	api->Mem->free(sender_stk);
	api->Mem->free(receiver_stk);

	in->unregisterIoEvent(in,1 << 0);

	api->Device->gpio->freeIo(out);
	api->Device->gpio->freeIo(in);

	return osOK;

}


static DECLARE_THREADROUTINE(sender){
	uint32_t cnt = 0;
	while(cnt < 50){
		sys->MsgQ->put(mid,0xFF,osWaitForever);
		out->out(out,1 << 2,bClear);
		sys->Thread->sleep(1);
		out->out(out,1 << 2,bSet);
		sys->Thread->sleep(1);
		cnt++;
	}
	sys->Barrier->wait(mBar,osWaitForever);
	sys->Thread->sleep(10);
	sys->MsgQ->destroy(mid);
	return osOK;
}

static DECLARE_THREADROUTINE(receiver){
	uint32_t cnt = 0;
	osEvent evt;
	uint32_t mval = 0;
	uint32_t totalMsgcnt = 0;
	while(cnt < 100){
		evt = sys->MsgQ->get(mid,osWaitForever);
		totalMsgcnt++;
		if(evt.status == osEventMessage){
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
	evt = sys->MsgQ->get(mid,10);
	if(evt.status != osErrorTimeoutResource)
		return osErrorOS;
	sys->Barrier->signal(mBar,osOK);
	evt = sys->MsgQ->get(mid,osWaitForever);
	if(evt.status != osErrorResource)
		return osErrorResource;
	return osOK;
}

static DECLARE_IO_CALLBACK(ioEventListener){
	Api->MsgQ->put(mid,0xF0,0);
	return TRUE;
}

