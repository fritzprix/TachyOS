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



static BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin);
tch_gpio_handle* led  = NULL;
tch_gpio_handle* btn  = NULL;

int main(tch* api) {

//	tch* api = (tch*) arg;

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

//	tchStatus (*registerIoEvent)(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t* pmsk,uint32_t timeout);

	uint32_t pmsk = 1 << 10;
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	btn->registerIoEvent(btn,&evcfg,&pmsk);

	tch_assert(api,gpio_performTest(api) == osOK,osErrorOS);

	tch_assert(api,mtx_performTest(api) == osOK,osErrorOS);
	tch_assert(api,semaphore_performTest(api) == osOK,osErrorOS);
	tch_assert(api,barrier_performTest(api) == osOK,osErrorOS);
	tch_assert(api,monitor_performTest(api) == osOK,osErrorOS);
	tch_assert(api,mpool_performTest(api) == osOK,osErrorOS);
	tch_assert(api,msgq_performTest(api) == osOK,osErrorOS);
	tch_assert(api,mailq_performTest(api) == osOK,osErrorOS);
	tch_assert(api,async_performTest(api) == osOK,osErrorOS);

//	tch_UartHandle* (*open)(tch* env,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = api->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = api->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;

	tch_UartHandle* seriwal = api->Device->usart->open(api,&ucfg,osWaitForever,ActOnSleep);
	uint8_t buf[100];
	while(1){
		seriwal->write(seriwal,"Serial Port Working",18);/*
		seriwal->read(seriwal,buf,10,osWaitForever);
		seriwal->write(seriwal,buf,10);
		led->out(led,(1 << 6),bSet);
		api->Thread->sleep(10);
		led->out(led,(1 << 6),bClear);
		api->Thread->sleep(10);
		btn->listen(btn,10,osWaitForever);*/
	}
	return osOK;
}


uint32_t prscnt = 0;
BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin){
	prscnt++;
	return TRUE;
}

