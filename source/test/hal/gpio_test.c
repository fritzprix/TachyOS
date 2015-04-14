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


	tch_threadCfg thcfg;
	thcfg.t_name = "evgen";
	thcfg.t_routine = evgenRoutine;
	thcfg.t_priority = Normal;
	thcfg.t_memDef.stk_sz = 1 << 9;
	tch_threadId evgenThread = api->Thread->create(&thcfg,api);

	thcfg.t_name = "evcons";
	thcfg.t_routine = evconsRoutine;
	tch_threadId evconsThread = api->Thread->create(&thcfg,api);

	api->Thread->start(evgenThread);
	api->Thread->start(evconsThread);


	tchStatus result = tchOK;

	if((result = api->Thread->join(evgenThread,tchWaitForever)) != tchOK)
		return tchErrorOS;
	if((result = api->Thread->join(evconsThread,tchWaitForever)) != tchOK)
		return tchErrorOS;

	return tchOK;
}



static DECLARE_THREADROUTINE(evgenRoutine){

	tch_GpioCfg iocfg;
	const tch_lld_gpio* gpio = env->Device->gpio;
	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_OD;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.Speed = GPIO_OSpeed_50M;
	tch_GpioHandle* out = gpio->allocIo(env,tch_gpio1,outp,&iocfg,tchWaitForever);    // set pin 8 @ port b as output


	out->out(out,outp,bSet);
	env->Thread->yield(50);
	out->out(out,outp,bClear);
//	out->close(out);
	return tchOK;
}

static DECLARE_THREADROUTINE(evconsRoutine){

	tch_GpioHandle* in = NULL;
	tch_GpioCfg iocfg;
	const tch_lld_gpio* gpio = env->Device->gpio;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	iocfg.Speed = GPIO_OSpeed_50M;
	in = gpio->allocIo(env,tch_gpio0,inp,&iocfg,tchWaitForever);  // set pin 2 @ port a as input

	tch_GpioEvCfg evcfg;
	gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	uint32_t evpin = inp;
	in->registerIoEvent(in,&evcfg,&evpin);

	if(!in->listen(in,2,tchWaitForever))
		return tchErrorOS;
	in->unregisterIoEvent(in,inp);
	if(in->listen(in,2,tchWaitForever))
		return tchErrorOS;
	in->unregisterIoEvent(in,inp);
//	in->close(in);
	return tchOK;
}
