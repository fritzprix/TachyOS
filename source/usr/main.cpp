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
#include <string>


typedef struct person{
	enum {male,female}sex;
	int age;
}person;

typedef struct classroom {
	person students[50];
}classroom;


tch_thread_id childThread;
static DECLARE_THREADROUTINE(childthread_routine);
static DECLARE_THREADSTACK(childthr_stack,1 << 11);

static BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin);
tch_gpio_handle* led  = NULL;
tch_gpio_handle* btn  = NULL;

int main(void* arg) {


	person* p = new person();
	delete p;
	tch* api = (tch*) arg;

	tch_gpio_cfg iocfg;
	tch_gpio_evCfg evcfg;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;


	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	led = api->Device->gpio->allocIo(gpIo_5,6,&iocfg,osWaitForever,NoActOnSleep);

	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.In;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	btn = api->Device->gpio->allocIo(gpIo_5,10,&iocfg,osWaitForever,NoActOnSleep);



	btn->registerIoEvent(btn,&evcfg,onBtnPressed);

	tch_thread_cfg thcfg;
	thcfg._t_name = "childthread";
	thcfg._t_routine = childthread_routine;
	thcfg._t_stack = childthr_stack;
	thcfg.t_stackSize = (1 << 11);
	thcfg.t_proior = High;
	childThread = api->Thread->create(&thcfg,arg);
	api->Thread->start(childThread);


	float fVal = 0.f;
	while(1){
		fVal += 0.02f;
		led->out(led,bSet);
		api->Thread->sleep(100);
		led->out(led,bClear);
		api->Thread->sleep(100);
		btn->listen(btn,osWaitForever);
		classroom* clp = new classroom();

		classroom* acls = (classroom*)malloc(sizeof(classroom));
		delete clp;
		free(acls);
	}
	return 0;
}

uint32_t prscnt = 0;
BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin){
	prscnt++;
	return TRUE;
}

DECLARE_THREADROUTINE(childthread_routine){
	tch* api = (tch*) arg;
	tch_gpio_cfg iocfg;
	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	iocfg.Speed = api->Device->gpio->Speed.Low;
	tch_gpio_handle* bled = api->Device->gpio->allocIo(gpIo_5,7,&iocfg,osWaitForever,ActOnSleep);

	while(1){
		bled->out(bled,bSet);
		api->Thread->sleep(20);
		bled->out(bled,bClear);
		api->Thread->sleep(20);
	}
	return 0;
}
