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

static DECLARE_THREADROUTINE(childRoutine);

int main(const tch* api) {


	tch_GpioCfg iocfg;
	tch_GpioEvCfg evcfg;
	api->Device->gpio->initEvCfg(&evcfg);
	api->Device->gpio->initCfg(&iocfg);


	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_PP;
	led = api->Device->gpio->allocIo(api,tch_gpio5,(1 << 6),&iocfg,osWaitForever,NoActOnSleep);

	api->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	btn = api->Device->gpio->allocIo(api,tch_gpio5,(1 << 10),&iocfg,osWaitForever,NoActOnSleep);

	tch_assert(api,TRUE,osErrorISR);


	uint32_t pmsk = 1 << 10;
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	btn->registerIoEvent(btn,&evcfg,&pmsk);


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
	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_PP;
	iocfg.Speed = GPIO_OSpeed_100M;
	tch_gpio_handle* out = NULL;

	tch_gptimerDef gptdef;
	gptdef.UnitTime = TIMER_UNITTIME_uSEC;
	gptdef.pwrOpt = ActOnSleep;

	tch_gptimerHandle* timer = NULL;
	uint32_t loopcnt = 0;
	tch_threadCfg thcfg;
	thcfg._t_name = "Child";
	thcfg._t_routine = childRoutine;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 10;
	tch_threadId nchild = api->Thread->create(&thcfg,api->Thread->self());
	api->Thread->start(nchild);

	while(1){
		timer = api->Device->timer->openGpTimer(api,tch_TIMER0,&gptdef,osWaitForever);
		out = api->Device->gpio->allocIo(api,tch_gpio2,(1 << 14),&iocfg,osWaitForever,ActOnSleep);
		out->out(out,1 << 14,bClear);
		timer->wait(timer,15);
		out->out(out,1 << 14,bSet);
		timer->wait(timer,15);
		out->out(out,1 << 14,bClear);
		if((loopcnt++ % 10000) == 0){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}
		timer->wait(timer,15);
		out->out(out,1 << 14,bSet);
		timer->wait(timer,20);
		timer->close(timer);
		out->close(out);
		if((loopcnt % 10000) == 5000){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}
	}
	return osOK;
}

static DECLARE_THREADROUTINE(childRoutine){
	tch_threadId parent = env->Thread->getArg();
	env->uStdLib->stdio->iprintf("\rChild Initiated \n");

	uint32_t cnt = 100;
	while(cnt){
		env->uStdLib->stdio->iprintf("\rChild Loop %x\n",cnt++);
		env->Thread->sleep(100);
	}
	env->Thread->terminate(parent,osOK);
	return osOK;
}





