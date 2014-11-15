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

uint16_t outp = 1 << 8;
uint16_t inp = 1 << 2;

tchStatus gpio_performTest(const tch* api){
//	uint8_t* evgen_stk = api->Mem->alloc(1 << 9);
//	uint8_t* wt_stk = api->Mem->alloc(1 << 9);

	tch_GpioCfg iocfg;
	const tch_lld_gpio* gpio = api->Device->gpio;
	iocfg.Mode = gpio->Mode.Out;
	iocfg.Otype = gpio->Otype.PushPull;
	iocfg.PuPd = gpio->PuPd.NoPull;
	iocfg.Speed = gpio->Speed.Mid;
	tch_GpioHandle* out = gpio->allocIo(api,gpio->Ports.gpio_1,outp,&iocfg,osWaitForever,ActOnSleep);    // set pin 8 @ port b as output

	gpio->initCfg(&iocfg);
	iocfg.Mode = gpio->Mode.In;
	iocfg.PuPd = gpio->PuPd.PullUp;
	iocfg.Speed = gpio->Speed.Mid;
	tch_GpioHandle* in = gpio->allocIo(api,gpio->Ports.gpio_0,inp,&iocfg,osWaitForever,ActOnSleep);  // set pin 2 @ port a as input

	tch_GpioEvCfg evcfg;
	gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = gpio->EvEdeg.Fall;
	evcfg.EvType = gpio->EvType.Interrupt;
	uint32_t evpin = inp;
	in->registerIoEvent(in,&evcfg,&evpin);

	tch_threadCfg thcfg;
	thcfg._t_name = "evgen";
	thcfg._t_routine = evgenRoutine;
//	thcfg._t_stack = evgen_stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;
	tch_threadId evgenThread = api->Thread->create(&thcfg,out);

	thcfg._t_name = "evcons";
	thcfg._t_routine = evconsRoutine;
//	thcfg._t_stack = wt_stk;
	tch_threadId evconsThread = api->Thread->create(&thcfg,in);

	api->Thread->start(evgenThread);
	api->Thread->start(evconsThread);



	if(api->Thread->join(evgenThread,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(evconsThread,osWaitForever) != osOK)
		return osErrorOS;

//	api->Mem->free(evgen_stk);
//	api->Mem->free(wt_stk);
	return osOK;
}



static DECLARE_THREADROUTINE(evgenRoutine){
	tch_GpioHandle* out = (tch_GpioHandle*) env->Thread->getArg();
	out->out(out,outp,bSet);
	env->Thread->sleep(50);
	out->out(out,outp,bClear);
	env->Device->gpio->freeIo(out);
	return osOK;
}

static DECLARE_THREADROUTINE(evconsRoutine){
	tch_GpioHandle* in = (tch_GpioHandle*) env->Thread->getArg();
	if(!in->listen(in,2,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in,inp);
	if(in->listen(in,2,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in,inp);
	env->Device->gpio->freeIo(in);
	return osOK;
}
