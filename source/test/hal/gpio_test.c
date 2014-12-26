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
	thcfg._t_name = "evgen";
	thcfg._t_routine = evgenRoutine;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;
	tch_threadId evgenThread = api->Thread->create(&thcfg,api);

	thcfg._t_name = "evcons";
	thcfg._t_routine = evconsRoutine;
	tch_threadId evconsThread = api->Thread->create(&thcfg,api);

	api->Thread->start(evgenThread);
	api->Thread->start(evconsThread);


	tchStatus result = osOK;

	if((result = api->Thread->join(evgenThread,osWaitForever)) != osOK)
		return osErrorOS;
	if((result = api->Thread->join(evconsThread,osWaitForever)) != osOK)
		return osErrorOS;

	return osOK;
}



static DECLARE_THREADROUTINE(evgenRoutine){

	tch_GpioCfg iocfg;
	const tch_lld_gpio* gpio = env->Device->gpio;
	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_OD;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.Speed = GPIO_OSpeed_50M;
	tch_GpioHandle* out = gpio->allocIo(env,tch_gpio1,outp,&iocfg,osWaitForever);    // set pin 8 @ port b as output


	out->out(out,outp,bSet);
	env->Thread->yield(50);
	out->out(out,outp,bClear);
//	out->close(out);
	return osOK;
}

static DECLARE_THREADROUTINE(evconsRoutine){

	tch_GpioHandle* in = NULL;
	tch_GpioCfg iocfg;
	const tch_lld_gpio* gpio = env->Device->gpio;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	iocfg.Speed = GPIO_OSpeed_50M;
	in = gpio->allocIo(env,tch_gpio0,inp,&iocfg,osWaitForever);  // set pin 2 @ port a as input

	tch_GpioEvCfg evcfg;
	gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	uint32_t evpin = inp;
	in->registerIoEvent(in,&evcfg,&evpin);

	if(!in->listen(in,2,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in,inp);
	if(in->listen(in,2,osWaitForever))
		return osErrorOS;
	in->unregisterIoEvent(in,inp);
//	in->close(in);
	return osOK;
}
