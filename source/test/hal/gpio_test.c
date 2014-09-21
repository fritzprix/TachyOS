/*
 * gpio_test.c
 *
 *  Created on: 2014. 9. 9.
 *      Author: manics99
 */


#include "tch.h"
#include "gpio_test.h"

static DECLARE_THREADROUTINE(evgenRoutine);
static DECLARE_THREADROUTINE(evconsRoutine);

tchStatus gpio_performTest(tch* api){
	uint8_t* evgen_stk = api->Mem->alloc(1 << 9);
	uint8_t* wt_stk = api->Mem->alloc(1 << 9);

	tch_gpio_cfg iocfg;
	tch_lld_gpio* gpio = api->Device->gpio;
	iocfg.Mode = gpio->Mode.Out;
	iocfg.Otype = gpio->Otype.PushPull;
	iocfg.PuPd = gpio->PuPd.NoPull;
	iocfg.Speed = gpio->Speed.Mid;
	tch_gpio_handle* out = gpio->allocIo(api,gpio->Ports.gpio_0,0,&iocfg,osWaitForever,ActOnSleep);

	gpio->initCfg(&iocfg);
	iocfg.Mode = gpio->Mode.In;
	iocfg.PuPd = gpio->PuPd.PullUp;
	iocfg.Speed = gpio->Speed.Mid;
	tch_gpio_handle* in = gpio->allocIo(api,gpio->Ports.gpio_0,2,&iocfg,osWaitForever,ActOnSleep);

	tch_gpio_evCfg evcfg;
	evcfg.EvEdge = gpio->EvEdeg.Fall;
	evcfg.EvType = gpio->EvType.Interrupt;
	in->registerIoEvent(in,&evcfg,NULL,osWaitForever);

	tch_threadCfg thcfg;
	thcfg._t_name = "evgen";
	thcfg._t_routine = evgenRoutine;
	thcfg._t_stack = evgen_stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;
	tch_threadId evgenThread = api->Thread->create(&thcfg,out);

	thcfg._t_name = "evcons";
	thcfg._t_routine = evconsRoutine;
	thcfg._t_stack = wt_stk;
	tch_threadId evconsThread = api->Thread->create(&thcfg,in);

	api->Thread->start(evgenThread);
	api->Thread->start(evconsThread);



	if(api->Thread->join(evgenThread,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(evconsThread,osWaitForever) != osOK)
		return osErrorOS;

	api->Mem->free(evgen_stk);
	api->Mem->free(wt_stk);
	return osOK;
}



static DECLARE_THREADROUTINE(evgenRoutine){
	tch_gpio_handle* out = (tch_gpio_handle*) sys->Thread->getArg();
	out->out(out,bSet);
	sys->Thread->sleep(50);
	out->out(out,bClear);
	sys->Device->gpio->freeIo(out);
	return osOK;
}

static DECLARE_THREADROUTINE(evconsRoutine){
	tch_gpio_handle* in = (tch_gpio_handle*) sys->Thread->getArg();
	if(!in->listen(in,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in);
	if(in->listen(in,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in);
	sys->Device->gpio->freeIo(in);
	return osOK;
}
