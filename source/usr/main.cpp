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

int main(const tch* api) {


	tch_mailqId adcReadQ = api->MailQ->create(100,2);

	tch_adcCfg adccfg;
	api->Device->adc->initCfg(&adccfg);
	adccfg.Precision = ADC_Precision_10B;
	adccfg.SampleFreq = 10000;
	adccfg.SampleHold = ADC_SampleHold_Short;
	api->Device->adc->addChannel(&adccfg.chdef,tch_ADC_Ch1);

	tch_pwmDef pwmDef;
	pwmDef.pwrOpt = ActOnSleep;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = 1000;

	tch_adcHandle* adc = api->Device->adc->open(api,tch_ADC1,&adccfg,ActOnSleep,osWaitForever);
	tch_pwmHandle* pwm = api->Device->timer->openPWM(api,tch_TIMER2,&pwmDef,osWaitForever);
	pwm->setOutputEnable(pwm,1,TRUE,osWaitForever);
	pwm->close(pwm);

	uint32_t loopcnt = 0;
	uint16_t* readb;
	osEvent evt;
	while(1){
		pwm = api->Device->timer->openPWM(api,tch_TIMER2,&pwmDef,osWaitForever);
		pwm->setOutputEnable(pwm,1,TRUE,osWaitForever);
		pwm->start(pwm);
		api->uStdLib->stdio->iprintf("\rRead Analog Value : %d\n",adc->read(adc,tch_ADC_Ch1));
		adc->readBurst(adc,tch_ADC_Ch1,adcReadQ,1);
		evt = api->MailQ->get(adcReadQ,osWaitForever);
		if(evt.status == osEventMail){
			readb = (uint16_t*) evt.value.p;
			api->MailQ->free(adcReadQ,readb);
		}
		if((loopcnt++ % 100) == 0){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}
		/**
		 * 	tchStatus (*write)(tch_pwmHandle* self,uint32_t ch,float* fduty,size_t sz);
		 *
		 */
		pwm->write(pwm,1,dutyArr,10);
		pwm->close(pwm);
		if((loopcnt % 100) == 50){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}
	}
	return osOK;
}




