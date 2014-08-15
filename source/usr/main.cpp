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


tch_thread_id childThread;
static DECLARE_THREADROUTINE(childthread_routine);
static DECLARE_THREADSTACK(childthr_stack,1 << 11);
static void race(tch* api);

volatile tch_mtx tmtx;

static BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin);
tch_gpio_handle* led  = NULL;
tch_gpio_handle* btn  = NULL;

static DECL_ASYNC_TASK(async_job);
tch_async_id async_id;
uint8_t mcnt;
static void dummyfunc(void* arg, uint32_t a);
void (*dummyfn)(void* arg,uint32_t a);
int main(void* arg) {

	dummyfn = dummyfunc;
	tch_mtx* mmtx = &tmtx;
	tch* api = (tch*) arg;

	api->Mtx->create(mmtx);

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


	while(1){
		led->out(led,bSet);
		api->Thread->sleep(100);
		for(mcnt = 0;mcnt < 200;mcnt++){
	//		race(api);
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK)
				api->Mtx->unlock(mmtx);
		}
		led->out(led,bClear);
		api->Thread->sleep(100);
		for(mcnt = 0;mcnt < 200;mcnt++){
			//race(api);
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK){
				api->Thread->sleep(0);
				api->Mtx->unlock(mmtx);
			}
		}
		for(mcnt = 0;mcnt < 200;mcnt++){
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK){
				api->Thread->sleep(0);
				api->Mtx->unlock(mmtx);
			}
		}


		btn->listen(btn,osWaitForever);
		dummyfn(mmtx,osWaitForever);

		for(mcnt = 0;mcnt < 200;mcnt++){
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK){
				api->Thread->sleep(0);
				api->Mtx->unlock(mmtx);
			}
		}
		classroom* clp = new classroom();
		classroom* acls = (classroom*)malloc(sizeof(classroom));
		delete clp;
		free(acls);
/*             Mtx Race 부분이 이 위치에 있을 때 종종 Mtx Parameter가 전달이 안되는 경우가 ㅅ애긴다.
 *
 */

	//	api->Thread->sleep(0);
	}
	return 0;
}

static void dummyfunc(void* arg, uint32_t a){
	if(arg == 0){
		a++;
	}
}


uint32_t prscnt = 0;
BOOL onBtnPressed(tch_gpio_handle* gpio,uint8_t pin){
	prscnt++;
	return TRUE;
}

uint8_t ccnt;
DECLARE_THREADROUTINE(childthread_routine){
	tch* api = (tch*) arg;
	tch_gpio_cfg iocfg;
	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = api->Device->gpio->Mode.Out;
	iocfg.Otype = api->Device->gpio->Otype.PushPull;
	iocfg.Speed = api->Device->gpio->Speed.Low;
	tch_gpio_handle* bled = api->Device->gpio->allocIo(gpIo_5,7,&iocfg,osWaitForever,ActOnSleep);

	tch_gpio_handle* btntoggle = api->Device->gpio->allocIo(gpIo_0,6,&iocfg,osWaitForever,ActOnSleep);

	tch_mtx* mmtx = &tmtx;

//	async_id = api->Async->create(async_job,arg,api->Async->Prior.Normal);
//	tchStatus result = api->Async->blockedstart(async_id,osWaitForever);
	while(1){
		bled->out(bled,bSet);
		api->Thread->sleep(20);
		for(ccnt = 0;ccnt < 200;ccnt++){
			btntoggle->out(btntoggle,bSet);
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK){
				btntoggle->out(btntoggle,bClear);
				api->Mtx->unlock(mmtx);
			}
		}
		bled->out(bled,bClear);
		api->Thread->sleep(20);
		for(ccnt = 0;ccnt < 200;ccnt++){
			btntoggle->out(btntoggle,bSet);
			if(api->Mtx->lock(mmtx,osWaitForever) == osOK)
				btntoggle->out(btntoggle,bClear);
				api->Mtx->unlock(mmtx);
		}
	}
	return 0;
}

void race(tch* api){
	if(api->Mtx->lock(&tmtx,osWaitForever) != osOK)
		return;
//	api->Thread->sleep(0);
	api->Mtx->unlock(&tmtx);
}


static DECL_ASYNC_TASK(async_job){
	((tch*)arg)->Async->notify(id,osOK);
	return TRUE;
}

