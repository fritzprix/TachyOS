/*
 * async_test.c
 *
 *  Created on: 2014. 8. 10.
 *      Author: innocentevil
 */


#include "async_test.h"


// test scenario
/**
 *  1 . create a thread
 *  2 . initialize gpio as interrupt
 *  3 . initialize gpio as output
 *  4 . create async
 *  5 . post & wait task,which trigger interrupt by handling previously initialized gpio
 *  6 . wake up by gpio interrupt
 */

tch_threadId testThread;
tch_threadId competitorThread;
tch_threadId destroyerThread;
tch_asyncId async;
tch* api;

static DECLARE_THREADROUTINE(testRoutine1);
static DECLARE_THREADROUTINE(destroyerRoutine);
static DECLARE_THREADROUTINE(competitorRoutine);
static DECLARE_IO_CALLBACK(testIoListener);

static DECL_ASYNC_TASK(testAsyncWorkingRoutine);
static DECL_ASYNC_TASK(testAsyncNotWorkingRoutine);



tchStatus async_performTest(tch* api){
	uint8_t* testThreadStk = api->Mem->alloc(1 << 9);
	tch_threadCfg thcfg;
	thcfg._t_name = "async_tester";
	thcfg._t_routine = testRoutine1;
	thcfg._t_stack = testThreadStk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	async = api->Async->create(10);
	testThread = api->Thread->create(&thcfg,api);
	api->Thread->start(testThread);
	if(api->Thread->join(testThread,osWaitForever) != osOK){
		return osErrorOS;
	}
	return osOK;

}



static DECLARE_THREADROUTINE(testRoutine1){
	tch_lld_gpio* gpio = sys->Device->gpio;
	api = sys;
	tch_GpioCfg iocfg;
	gpio->initCfg(&iocfg);
	iocfg.Mode = gpio->Mode.In;
	iocfg.PuPd = gpio->PuPd.PullUp;
	iocfg.Speed = gpio->Speed.Mid;

	tch_GpioHandle* in = gpio->allocIo(sys,gpio->Ports.gpio_0,(1 << 0),&iocfg,osWaitForever,ActOnSleep);

	gpio->initCfg(&iocfg);
	iocfg.Mode = gpio->Mode.Out;
	iocfg.Otype = gpio->Otype.PushPull;
	iocfg.PuPd = gpio->PuPd.PullUp;
	iocfg.Speed = gpio->Speed.Mid;

	tch_GpioHandle* out = gpio->allocIo(sys,gpio->Ports.gpio_0,(1 << 2),&iocfg,osWaitForever,ActOnSleep);

	tch_GpioEvCfg evcfg;
	gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = gpio->EvEdeg.Fall;
	evcfg.EvType = gpio->EvType.Interrupt;
	evcfg.EvCallback = testIoListener;
	uint32_t pmsk = 1 << 0;
	in->registerIoEvent(in,&evcfg,&pmsk);
	if(!async)
		return osErrorOS;
	if(sys->Async->wait(async,async,testAsyncNotWorkingRoutine,10,out) == osOK)
		return osErrorOS;
	if(sys->Async->wait(async,async,testAsyncWorkingRoutine,osWaitForever,out) != osOK)
		return osErrorOS;
	//create child thread to destroy async

	uint8_t* destroyerStk = sys->Mem->alloc(1 << 9);
	uint8_t* competitorStk = sys->Mem->alloc(1 << 9);


	tch_threadCfg thcfg;
	thcfg._t_name = "dest";
	thcfg._t_routine = destroyerRoutine;
	thcfg._t_stack = destroyerStk;
	thcfg.t_stackSize = 1 << 9;
	thcfg.t_proior = Normal;
	destroyerThread = sys->Thread->create(&thcfg,async);


	thcfg._t_name = "Competitor";
	thcfg._t_routine = competitorRoutine;
	thcfg._t_stack = competitorStk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;
	competitorThread = sys->Thread->create(&thcfg,out);

	sys->Thread->start(competitorThread);

	if(sys->Async->wait(async,async,testAsyncWorkingRoutine,osWaitForever,out) != osOK)
		return osErrorOS;


	sys->Thread->start(destroyerThread);
	tchStatus result = osOK;
	if((result = sys->Async->wait(async,testThread,testAsyncNotWorkingRoutine,osWaitForever,out)) == osOK)   // blocked by async request
		return osErrorOS;

	result = sys->Thread->join(competitorThread,osWaitForever);
	sys->Mem->free(destroyerStk);
	sys->Mem->free(competitorStk);

	sys->Device->gpio->freeIo(in);
	sys->Device->gpio->freeIo(out);

	return result;
}


static DECLARE_THREADROUTINE(destroyerRoutine){
	sys->Thread->sleep(10);
	sys->Async->destroy((tch_asyncId)sys->Thread->getArg());   // destroy and wakeup waiting thread
	return osOK;
}

static DECLARE_THREADROUTINE(competitorRoutine){
	tch_GpioHandle* out = (tch_GpioHandle*)sys->Thread->getArg();
	if(sys->Async->wait(async,async,testAsyncWorkingRoutine,osWaitForever,out) != osOK)
		return osErrorOS;
	if(sys->Async->wait(async,competitorThread,testAsyncNotWorkingRoutine,osWaitForever,sys) != osErrorResource)   // blocked by async request
		return osErrorOS;
	return osOK;
}



static DECLARE_IO_CALLBACK(testIoListener){
	if(!api)
		return FALSE;
	api->Async->notify(async,async,osOK);
	return TRUE;
}

static DECL_ASYNC_TASK(testAsyncWorkingRoutine){
	tch_GpioHandle* out = (tch_GpioHandle*) arg;
	out->out(out,1 << 2,bClear);     // trigger interrupt
	out->out(out,1 << 2,bSet);
	return osOK;
}

static DECL_ASYNC_TASK(testAsyncNotWorkingRoutine){
	return osOK;
}

