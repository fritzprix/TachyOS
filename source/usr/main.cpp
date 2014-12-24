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

float dutyArr[10] = {
		0.1f,
		0.2f,
		0.3f,
		0.4f,
		0.5f,
		0.6f,
		0.7f,
		0.8f,
		0.9f,
		0.99f
};

const uint8_t vars[10] = {
		0,1,2,3,4,5,6,7,8,9
};

const uint8_t MO_CTRL_REG1 = 0x20;
const uint8_t MO_CTRL_REG2 = 0x21;
const uint8_t MO_CTRL_REG3 = 0x22;
const uint8_t MO_CTRL_REG4 = 0x23;
const uint8_t MO_CTRL_REG5 = 0x24;

const uint8_t MO_OUT_X_L = 0x28;
uint16_t msAddr = 0x1D2;

static DECLARE_THREADROUTINE(childThreadRoutine);
static DECLARE_THREADROUTINE(btnHandler);


int main(const tch* env) {




	tch_threadCfg thcfg;
	env->uStdLib->string->memset(&thcfg,0,sizeof(tch_threadCfg));
	thcfg._t_name = "child1";
	thcfg._t_routine = childThreadRoutine;
	thcfg.t_stackSize = 512;
	tch_threadId childId = env->Thread->create(&thcfg,NULL);

	thcfg._t_name = "btnHandler";
	thcfg._t_routine = btnHandler;
	thcfg.t_stackSize = 512;
	tch_threadId btnHandler = env->Thread->create(&thcfg,NULL);

	env->Thread->start(childId);
	env->Thread->start(btnHandler);


	tch_pwmDef pwmDef;
	pwmDef.pwrOpt = ActOnSleep;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = 1000;

	tch_pwmHandle* pwm = env->Device->timer->openPWM(env,tch_TIMER2,&pwmDef,osWaitForever);
	pwm->setOutputEnable(pwm,1,TRUE,osWaitForever);
	pwm->close(pwm);

	tch_iicCfg iicCfg;
	env->Device->i2c->initCfg(&iicCfg);
	iicCfg.Addr = 0xD2;
	iicCfg.AddrMode = IIC_ADDRMODE_7B;
	iicCfg.Baudrate = IIC_BAUDRATE_HIGH;
	iicCfg.Filter = TRUE;
	iicCfg.Role = IIC_ROLE_MASTER;
	iicCfg.OpMode = IIC_OPMODE_FAST;


	tch_iicHandle* iic = env->Device->i2c->allocIIC(env,IIc1,&iicCfg,osWaitForever,ActOnSleep);

	uint32_t loopcnt = 0;
	uint8_t buf[10];
	env->uStdLib->string->memset(buf,0,sizeof(uint8_t) * 10);

	buf[0] = MO_CTRL_REG4;
	buf[1] = (1 << 7) | (1 << 4);
	iic->write(iic,msAddr,buf,2);

	iic->write(iic,msAddr,&MO_CTRL_REG4,1);
	iic->read(iic,msAddr,buf,1,osWaitForever);
	env->uStdLib->stdio->iprintf("\rRead Value : %d\n",buf[0]);

	buf[0] = MO_CTRL_REG1;
	buf[1] = ((1 << 3) | 7);
	iic->write(iic,msAddr,buf,2);

	uint8_t datareadAddr = (MO_OUT_X_L | 128);


	while(1){
		/*	tchStatus (*write)(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz);
		 * 		 */
		iic->write(iic,msAddr,&datareadAddr,1);
		iic->read(iic,msAddr,buf,6,osWaitForever);
		env->uStdLib->stdio->iprintf("\rRead Motion X : %d, Y : %d, Z : %d\n",(*(int16_t*)&buf[0]),(*(int16_t*)&buf[2]),(*(int16_t*)&buf[4]));
		env->Thread->sleep(1000);

		if((loopcnt++ % 1000) == 0){
			env->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",env->Mem->avail());
			env->Mem->printAllocList();
			env->Mem->printFreeList();
		}
		/**
		 *
		 */
		if((loopcnt % 1000) == 500){
			env->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",env->Mem->avail());
			env->Mem->printAllocList();
			env->Mem->printFreeList();
		}
	}
	return osOK;
}


static DECLARE_THREADROUTINE(btnHandler){
	tch_GpioCfg iocfg;
	env->Device->gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_IN;
	iocfg.PuPd = GPIO_PuPd_PU;
	tch_GpioHandle* iohandle = env->Device->gpio->allocIo(env,tch_gpio5,(1 << 13),&iocfg,osWaitForever);

	tch_GpioEvCfg evcfg;
	env->Device->gpio->initEvCfg(&evcfg);
	evcfg.EvEdge = GPIO_EvEdge_Fall;
	evcfg.EvType = GPIO_EvType_Interrupt;
	iohandle->registerIoEvent(iohandle,13,&evcfg);

	while(TRUE){
		if(iohandle->listen(iohandle,13,osWaitForever))
			env->uStdLib->stdio->iprintf("\rButton Pressed\n");
	}

	return osOK;
}


static DECLARE_THREADROUTINE(childThreadRoutine){
	tch_mailqId adcReadQ = env->MailQ->create(100,2);
	osEvent evt;
	env->uStdLib->string->memset(&evt,0,sizeof(osEvent));
	tch_adcCfg adccfg;
	env->Device->adc->initCfg(&adccfg);
	adccfg.Precision = ADC_Precision_10B;
	adccfg.SampleFreq = 10000;
	adccfg.SampleHold = ADC_SampleHold_Short;
	env->Device->adc->addChannel(&adccfg.chdef,tch_ADC_Ch1);

	tch_adcHandle* adc = env->Device->adc->open(env,tch_ADC1,&adccfg,ActOnSleep,osWaitForever);

	while(TRUE){
		env->uStdLib->stdio->iprintf("\rRead Analog Value : %d\n",adc->read(adc,tch_ADC_Ch1));
		adc->readBurst(adc,tch_ADC_Ch1,adcReadQ,1);
		evt = env->MailQ->get(adcReadQ,osWaitForever);
		if(evt.status == osEventMail){
			env->MailQ->free(adcReadQ,evt.value.p);
		}
		env->Thread->sleep(1000);
	}
	return osOK;
}




