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
#include "tch.h"
#include <stdio.h>
#include <stdlib.h>

#include "mtx_test.h"



static BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin);
tch_gpio_handle* led  = NULL;
tch_gpio_handle* btn  = NULL;

int main(void* arg) {

	tch* api = (tch*) arg;

	tch_gpio_cfg iocfg;
	tch_gpio_evCfg evcfg;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;


	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	led = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_5,6,&iocfg,osWaitForever,NoActOnSleep);

	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.In;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	btn = api->Device->gpio->allocIo(api,api->Device->gpio->Ports.gpio_5,10,&iocfg,osWaitForever,NoActOnSleep);



	btn->registerIoEvent(btn,&evcfg,onBtnPressed);

	if(mtx_performTest(api) != osOK)
		return osErrorOS;


	while(1){
		led->out(led,bSet);
		api->Thread->sleep(100);
		led->out(led,bClear);
		api->Thread->sleep(100);
		btn->listen(btn,osWaitForever);
	}
	return osOK;
}


uint32_t prscnt = 0;
BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin){
	prscnt++;
	return TRUE;
}



static DECL_ASYNC_TASK(async_job){
	((tch*)arg)->Async->notify(id,osOK);
	return TRUE;
}

