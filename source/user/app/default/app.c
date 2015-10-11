/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"
#include "user/module/libc/tch_ulibc.h"

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
		0.95f
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
uint16_t msAddr = 0xD2;

static DECLARE_THREADROUTINE(childThreadRoutine);
static DECLARE_THREADROUTINE(btnHandler);

tch_threadId btnHandleThread;
tch_threadId childId;

int main(const tch* ctx) {

/*
	tch_spiCfg spicfg;
	spicfg.Baudrate = SPI_BAUDRATE_HIGH;
	spicfg.Role = SPI_ROLE_MASTER;
	spicfg.ClkMode = SPI_CLKMODE_0;
	spicfg.FrmFormat = SPI_FRM_FORMAT_8B;
	spicfg.FrmOrient = SPI_FRM_ORI_LSBFIRST;

	ctx->Service->request(MODULE_TYPE_SPI);

	tch_spiHandle* spi = ctx->Device->spi->allocSpi(ctx,tch_spi0,&spicfg,tchWaitForever,ActOnSleep);

	tch_threadCfg thcfg;
	ctx->uStdLib->string->memset(&thcfg,0,sizeof(tch_threadCfg));
	ctx->Thread->initCfg(&thcfg,childThreadRoutine,Normal,720,0,"child1");
	childId = ctx->Thread->create(&thcfg,spi);

	ctx->Thread->initCfg(&thcfg,btnHandler,Normal,720,0,"btnHandler");
	btnHandleThread = ctx->Thread->create(&thcfg,spi);

	ctx->Thread->start(childId);
	ctx->Thread->start(btnHandleThread);


	tch_pwmDef pwmDef;
	pwmDef.pwrOpt = ActOnSleep;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.PeriodInUnitTime = 1000;

	tch_pwmHandle* pwm = ctx->Device->timer->openPWM(ctx,tch_TIMER2,&pwmDef,tchWaitForever);
	pwm->setOutputEnable(pwm,1,TRUE,tchWaitForever);

	tch_iicCfg iicCfg;
	ctx->Device->i2c->initCfg(&iicCfg);
	iicCfg.Addr = 0xD2;
	iicCfg.AddrMode = IIC_ADDRMODE_7B;
	iicCfg.Baudrate = IIC_BAUDRATE_HIGH;
	iicCfg.Filter = TRUE;
	iicCfg.Role = IIC_ROLE_MASTER;
	iicCfg.OpMode = IIC_OPMODE_FAST;


	tch_iicHandle* iic = ctx->Device->i2c->allocIIC(ctx,IIc2,&iicCfg,tchWaitForever,ActOnSleep);

	uint32_t loopcnt = 0;
	uint8_t buf[10];
	ctx->uStdLib->string->memset(buf,0,sizeof(uint8_t) * 10);
	tchStatus result = tchOK;

	buf[0] = MO_CTRL_REG4;
	buf[1] = (1 << 7) | (1 << 4);
	result = iic->write(iic,msAddr,buf,2);

	iic->write(iic,msAddr,&MO_CTRL_REG4,1);
	result = iic->read(iic,msAddr,buf,1,tchWaitForever);
//	ctx->uStdLib->stdio->iprintf("\rRead Value : %d\n",buf[0]);

	buf[0] = MO_CTRL_REG1;
	buf[1] = ((1 << 3) | 7);
	iic->write(iic,msAddr,buf,2);

	uint8_t datareadAddr = (MO_OUT_X_L | 128);
	int cnt = 0;
	int16_t x,y,z;
	while(TRUE){
		iic->write(iic,msAddr,&datareadAddr,1);
		iic->read(iic,msAddr,buf,9,tchWaitForever);
//		ctx->uStdLib->stdio->iprintf("\rMotion X  : %d, Y  : %d, Z  : %d\n",(*(int16_t*)&buf[0]),(*(int16_t*)&buf[2]),(*(int16_t*)&buf[4]));
		x = (*(int16_t*)&buf[0]);
		y = ((*(int16_t*)&buf[2]));
		z = (*(int16_t*)&buf[4]);
		pwm->start(pwm);
		pwm->write(pwm,1,dutyArr,10);
		pwm->stop(pwm);
		if((loopcnt++ % 1000) == 0){
	//		ctx->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",ctx->Mem->available());
		}
		spi->write(spi,"Hello World,Im the main!!!",16);
		if((loopcnt % 1000) == 500){
	//		ctx->uStdLib->stdio->iprintf("\r\nHeap Available Sizes : %d bytes\n",ctx->Mem->available());
		}
		ctx->Thread->sleep(1);
	}
	return tchOK;*/
	while(TRUE){
		ctx->Thread->sleep(2);
	}
	return tchOK;
}


static DECLARE_THREADROUTINE(btnHandler){
/*
	tch_spiHandle* spi = (tch_spiHandle*) ctx->Thread->getArg();
	char c;

	while(TRUE){
		spi->write(spi,"Press Button",11);
//		ctx->uStdLib->stdio->iprintf("\rButton Loop\n");
		ctx->Thread->sleep(2);
	}*/
	return tchOK;
}


static DECLARE_THREADROUTINE(childThreadRoutine){
/*	tch_spiHandle* spi = (tch_spiHandle*)  ctx->Thread->getArg();
	tch_mailqId adcReadQ = ctx->MailQ->create(100,2);
	struct tm ltm;
	tchEvent evt;
	ctx->uStdLib->string->memset(&evt,0,sizeof(tchEvent));
	tch_adcCfg adccfg;
	ctx->Device->adc->initCfg(&adccfg);
	adccfg.Precision = ADC_Precision_10B;
	adccfg.SampleFreq = 10000;
	adccfg.SampleHold = ADC_SampleHold_Short;
	ctx->Device->adc->addChannel(&adccfg.chdef,tch_ADC_Ch5);


	tch_adcHandle* adc = ctx->Device->adc->open(ctx,tch_ADC1,&adccfg,ActOnSleep,tchWaitForever);

	while(TRUE){
//		ctx->uStdLib->stdio->iprintf("\rRead Analog Value : %d \n",adc->read(adc,tch_ADC_Ch5));
		adc->readBurst(adc,tch_ADC_Ch5,adcReadQ,100,1);
		evt = ctx->MailQ->get(adcReadQ,tchWaitForever);
		if(evt.status == tchEventMail){
			ctx->MailQ->free(adcReadQ,evt.value.p);
		}

		ctx->Time->getLocaltime(&ltm);
//		ctx->uStdLib->stdio->iprintf("\r\n%d/%d/%d %d:%d:%d\n",ltm.tm_year + 1900,ltm.tm_mon + 1,ltm.tm_mday,ltm.tm_hour,ltm.tm_min,ltm.tm_sec);
		ctx->Thread->sleep(1);
		spi->write(spi,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz",50);
	}*/
	return tchOK;
}




