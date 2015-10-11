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
	api->Thread->initCfg(&thcfg,evgenRoutine,Normal,1 << 9,0,"evgen");
	tch_threadId evgenThread = api->Thread->create(&thcfg,api);

	api->Thread->initCfg(&thcfg,evconsRoutine,Normal,1 << 9,0,"evcons");
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

	gpio_config_t iocfg;
	tch_lld_gpio* gpio = ctx->Service->request(MODULE_TYPE_GPIO);
	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_OD;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.Speed = GPIO_OSpeed_50M;
	tch_gpioHandle* out = gpio->allocIo(ctx,tch_gpio1,outp,&iocfg,tchWaitForever);    // set pin 8 @ port b as output


	out->out(out,outp,bSet);
	ctx->Thread->yield(50);
	out->out(out,outp,bClear);
//	out->close(out);
	return tchOK;
}

static DECLARE_THREADROUTINE(evconsRoutine){

	tch_gpioHandle* in = NULL;
	gpio_config_t iocfg;
	tch_lld_gpio* gpio;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	iocfg.Speed = GPIO_OSpeed_50M;
	in = gpio->allocIo(ctx,tch_gpio0,inp,&iocfg,tchWaitForever);  // set pin 2 @ port a as input

	gpio_event_config_t evcfg;
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
