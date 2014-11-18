/*
 * main.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"



tch_gpio_handle* led  = NULL;
tch_gpio_handle* btn  = NULL;

int main(const tch* api) {



	tch_GpioCfg iocfg;
	tch_GpioEvCfg evcfg;
	api->Device->gpio->initEvCfg(&evcfg);
	api->Device->gpio->initCfg(&iocfg);


	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	led = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_5,(1 << 6),&iocfg,osWaitForever,NoActOnSleep);

	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.In;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	btn = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_5,(1 << 10),&iocfg,osWaitForever,NoActOnSleep);

	tch_assert(api,TRUE,osErrorISR);


	uint32_t pmsk = 1 << 10;
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	btn->registerIoEvent(btn,&evcfg,&pmsk);

	tch_assert(api,gpio_performTest(api) == osOK,osErrorOS);

//	tch_assert(api,mtx_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,semaphore_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,barrier_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,monitor_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,mpool_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,msgq_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,mailq_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,async_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,uart_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,timer_performTest(api) == osOK,osErrorOS);
//	tch_assert(api,spi_performTest(api) == osOK,osErrorOS);


	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	iocfg.Speed = api->Device->gpio->Speed.VeryHigh;
	tch_gpio_handle* out = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_2,(1 << 14),&iocfg,osWaitForever,ActOnSleep);
	out->out(out,1 << 3,bSet);

	tch_gptimerDef gptdef;
	gptdef.UnitTime = api->Device->timer->UnitTime.uSec;
	gptdef.pwrOpt = ActOnSleep;

	tch_gptimerHandle* timer = api->Device->timer->openGpTimer(api,api->Device->timer->timer.timer0,&gptdef,osWaitForever);

	while(1){
		out->out(out,1 << 14,bClear);
		timer->wait(timer,150);
		out->out(out,1 << 14,bSet);
		timer->wait(timer,150);
		out->out(out,1 << 14,bClear);
//		api->uStdLib->stdio->iprintf("\rThis is My Age : %d\n",35);
		timer->wait(timer,150);
		api->Thread->sleep(1);
		out->out(out,1 << 14,bSet);
		timer->wait(timer,200);
	}
	return osOK;
}

