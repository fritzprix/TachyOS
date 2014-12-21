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

	tch_iicCfg iicCfg;
	api->Device->i2c->initCfg(&iicCfg);
	iicCfg.Addr = 0xD2;
	iicCfg.AddrMode = IIC_ADDRMODE_7B;
	iicCfg.Baudrate = IIC_BAUDRATE_HIGH;
	iicCfg.Filter = TRUE;
	iicCfg.Role = IIC_ROLE_MASTER;
	iicCfg.OpMode = IIC_OPMODE_FAST;

	tch_iicHandle* iic = api->Device->i2c->allocIIC(api,IIc1,&iicCfg,osWaitForever,ActOnSleep);

	uint32_t loopcnt = 0;
	uint16_t* readb;
	osEvent evt;
	uint8_t buf[10];
	api->uStdLib->string->memset(buf,0,sizeof(uint8_t) * 10);

	buf[0] = MO_CTRL_REG4;
	buf[1] = (1 << 7) | (1 << 4);
	iic->write(iic,msAddr,buf,2);

	iic->write(iic,msAddr,&MO_CTRL_REG4,1);
	iic->read(iic,msAddr,buf,1,osWaitForever);
	api->uStdLib->stdio->iprintf("\rRead Value : %d\n",buf[0]);
/*
	buf[0] = MO_CTRL_REG5;
	buf[1] = 1 << 6;
	iic->write(iic,msAddr,buf,2);*/

	buf[0] = MO_CTRL_REG1;
	buf[1] = ((1 << 3) | 7);
	iic->write(iic,msAddr,buf,2);

	uint8_t datareadAddr = (MO_OUT_X_L | 128);


	while(1){
		/*	tchStatus (*write)(tch_iicHandle* self,uint16_t addr,const void* wb,size_t sz);
		 * 		 */
		iic->write(iic,msAddr,&datareadAddr,1);
		iic->read(iic,msAddr,buf,6,osWaitForever);
		api->uStdLib->stdio->iprintf("\rRead Motion X : %d, Y : %d, Z : %d\n",(*(int16_t*)&buf[0]),(*(int16_t*)&buf[2]),(*(int16_t*)&buf[4]));
//		api->uStdLib->stdio->iprintf("\rRead Analog Value : %d\n",adc->read(adc,tch_ADC_Ch1));
		api->Thread->sleep(100);
		adc->readBurst(adc,tch_ADC_Ch1,adcReadQ,1);
		evt = api->MailQ->get(adcReadQ,osWaitForever);
		if(evt.status == osEventMail){
			readb = (uint16_t*) evt.value.p;
			api->MailQ->free(adcReadQ,readb);
		}
		/*
		if((loopcnt++ % 100) == 0){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}*/
		/**
		 *
		 */
		/*
		if((loopcnt % 100) == 50){
			api->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",api->Mem->avail());
			api->Mem->printAllocList();
			api->Mem->printFreeList();
		}*/
	}
	return osOK;
}




