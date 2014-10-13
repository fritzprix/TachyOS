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


static void loop();
int main(tch* api) {


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
	tch_assert(api,uart_performTest(api) == osOK,osErrorOS);
	tch_assert(api,timer_performTest(api) == osOK,osErrorOS);

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = api->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = api->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;


	tch_UartHandle* serial = NULL;
	const char* msg = "This is Msg\n\r";
	int size = api->uStdLib->string->strlen(msg);
	while(1){
		serial = api->Device->usart->open(api,&ucfg,osWaitForever,ActOnSleep);
		serial->write(serial,msg,size);
		api->Device->usart->close(serial);
		api->Thread->sleep(1);
	}
	return osOK;
}

