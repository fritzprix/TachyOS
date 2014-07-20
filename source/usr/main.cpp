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


typedef struct person{
	enum {male,female}sex;
	int age;
}person;

typedef struct classroom {
	person students[50];
}classroom;

int main(void* arg) {
	tch* api = (tch*) arg;
	person* p = new person();
	api->Mem->free(p);
	tch_gpio_cfg iocfg;
	api->Device->gpio->initCfg(&iocfg);
	iocfg.Otype = PushPull;
	tch_gpio_handle* led = api->Device->gpio->allocIo(gpIo_5,6,&iocfg,NoActOnSleep);
	float fVal = 0.1f;
	while(1){
		fVal += 0.02f;
		led->out(led,bSet);
		api->Thread->sleep(1000);
		led->out(led,bClear);
		api->Thread->sleep(1000);
		classroom* clp = new classroom();
		delete clp;
	}
	return 0;
}

