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


typedef struct person{
	enum {male,female}sex;
	int age;
}person;

typedef struct classroom {
	person students[50];
}classroom;


static BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin);
tch_gpio_handle* led  = NULL;

int main(void* arg) {
	tch* api = (tch*) arg;
	person* p = new person();
	delete p;
	tch_gpio_cfg iocfg;
	tch_gpio_evCfg evcfg;
	evcfg.EvType = api->Device->gpio->EvType.Interrupt;
	evcfg.EvEdge = api->Device->gpio->EvEdeg.Fall;


	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	led = api->Device->gpio->allocIo(gpIo_5,6,&iocfg,NoActOnSleep);

	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.In;
	iocfg.PuPd = api->Device->gpio->PuPd.PullUp;
	tch_gpio_handle* btn = api->Device->gpio->allocIo(gpIo_5,10,&iocfg,NoActOnSleep);

	btn->registerIoEvent(btn,&evcfg,onBtnPressed);




	float fVal = 0.1f;
	while(1){
		fVal += 0.02f;
		led->out(led,bSet);
		api->Thread->sleep(100);
		api->Thread->sleep(100);
		classroom* clp = new classroom();
//		classroom* acls = (classroom*)malloc(sizeof(classroom));
		delete clp;
//		free(acls);
	}
	return 0;
}

uint32_t prscnt = 0;
BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin){
	prscnt++;
	led->out(led,bClear);
	return TRUE;
}

