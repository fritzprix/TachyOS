/*
 * timer_test.c
 *
 *  Created on: 2014. 10. 13.
 *      Author: innocentevil
 */


#include "tch.h"
#include "timer_test.h"


static DECLARE_THREADROUTINE(waiter1Run);
static DECLARE_THREADROUTINE(waiter2Run);

static DECLARE_THREADROUTINE(pulsDrv1Run);
static DECLARE_THREADROUTINE(pulsDrv2Run);

static DECLARE_THREADROUTINE(pulseGenRun);
static DECLARE_THREADROUTINE(pulseCon1Run);
static DECLARE_THREADROUTINE(pulseCon2Run);
static float fvs[1000];
static uint32_t ffvs[1000];
static uint32_t ffvs1[1000];

tchStatus timer_performTest(tch* ctx){

	tchStatus result = tchOK;

	// ************* Start of test for Basic Timer Function ********************** //
	tch_gptimerDef gptDef;
	gptDef.UnitTime = TIMER_UNITTIME_mSEC;
	gptDef.pwrOpt = ActOnSleep;

	tch_gptimerHandle* gptimer = ctx->Device->timer->openGpTimer(ctx,tch_TIMER0,&gptDef,tchWaitForever);

	tch_threadId waiterThread1;
	tch_threadId waiterThread2;


	tch_threadCfg thcfg;
	ctx->Thread->initCfg(&thcfg,waiter1Run,Normal,(1 << 9),0,"waiter1");
	waiterThread1 = ctx->Thread->create(&thcfg,gptimer);


	ctx->Thread->initCfg(&thcfg,waiter2Run,Normal,(1 << 9),0,"waiter2");
	waiterThread2 = ctx->Thread->create(&thcfg,gptimer);

	ctx->Thread->start(waiterThread1);
	ctx->Thread->start(waiterThread2);

	result = ctx->Thread->join(waiterThread1,tchWaitForever);
	result = ctx->Thread->join(waiterThread2,tchWaitForever);

	gptimer->close(gptimer);   // gptimer test complete

	// ************* End of test for Basic Timer Function ********************** //


	if(result != tchOK)
		return tchErrorOS;


	// ************* Start of test for PWM Out Function ********************** //

	tch_pwmDef pwmDef;
	pwmDef.PeriodInUnitTime = 1000;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.pwrOpt = ActOnSleep;
	int cnt = 100000;
	tch_pwmHandle* pwmDrv = NULL;
	while(cnt--){
		pwmDrv = ctx->Device->timer->openPWM(ctx,tch_TIMER0,&pwmDef,tchWaitForever);
		if(pwmDrv)
			pwmDrv->close(pwmDrv);
	}

	pwmDrv = ctx->Device->timer->openPWM(ctx,tch_TIMER0,&pwmDef,tchWaitForever);
	ctx->Thread->initCfg(&thcfg,pulsDrv1Run,Normal,(1 << 9),0,"pulsedrv1");
	waiterThread1 = ctx->Thread->create(&thcfg,pwmDrv);



	ctx->Thread->initCfg(&thcfg,pulsDrv2Run,Normal,(1 << 9),0,"pulsedrv2");
	waiterThread2 = ctx->Thread->create(&thcfg,pwmDrv);

	ctx->Thread->start(waiterThread1);
	ctx->Thread->start(waiterThread2);

	result = ctx->Thread->join(waiterThread1,tchWaitForever);
	result = ctx->Thread->join(waiterThread2,tchWaitForever);




	pwmDrv->close(pwmDrv);


	// ************* End of test for PWM Out Function ********************** //

	pwmDef.PeriodInUnitTime = 1000;
	pwmDef.UnitTime = TIMER_UNITTIME_uSEC;
	pwmDef.pwrOpt = ActOnSleep;

	pwmDrv = ctx->Device->timer->openPWM(ctx,tch_TIMER2,&pwmDef,tchWaitForever);
	if(!pwmDrv)
		return tchErrorOS;

	tch_tcaptDef captDef;
	captDef.Polarity = TIMER_POLARITY_NEGATIVE;
	captDef.UnitTime = TIMER_UNITTIME_uSEC;
	captDef.periodInUnitTime = 1000;
	captDef.pwrOpt = ActOnSleep;

	tch_tcaptHandle* capt = ctx->Device->timer->openTimerCapture(ctx,tch_TIMER0,&captDef,tchWaitForever);
	if(!capt)
		return tchErrorOS;

	ctx->Thread->initCfg(&thcfg,pulseGenRun,Normal,(1 << 9),0,"pgen");
	tch_threadId pgenThread = ctx->Thread->create(&thcfg,pwmDrv);

	ctx->Thread->initCfg(&thcfg,pulseCon1Run,Normal,(1 << 9),0,"pcon1");
	tch_threadId pcon1Thread = ctx->Thread->create(&thcfg,capt);

	uint8_t* pconStk = ctx->Mem->alloc(1 << 9);

	ctx->Thread->initCfg(&thcfg,pulseCon2Run,Normal,(1 << 9),0,"pcon2");
	tch_threadId pcon2Thread = ctx->Thread->create(&thcfg,capt);

	ctx->Thread->start(pgenThread);
	ctx->Thread->start(pcon1Thread);
	ctx->Thread->start(pcon2Thread);

	ctx->Thread->join(pgenThread,tchWaitForever);
	ctx->Thread->join(pcon1Thread,tchWaitForever);
	ctx->Thread->join(pcon2Thread,tchWaitForever);

	ctx->Mem->free(pconStk);

	capt->close(capt);
	pwmDrv->close(pwmDrv);

	// ************* Start of test for PWM Input Function ****************** //



	return result;
}


static DECLARE_THREADROUTINE(waiter1Run){
	tch_gptimerHandle* gptimer = (tch_gptimerHandle*)ctx->Thread->getArg();
	int cnt = 1000;
	while(cnt--)
		gptimer->wait(gptimer,10);
	return tchOK;

}

static DECLARE_THREADROUTINE(waiter2Run){
	tch_gptimerHandle* gptimer = (tch_gptimerHandle*)ctx->Thread->getArg();
	int cnt = 1000;
	while(cnt--)
		gptimer->wait(gptimer,10);
	return tchOK;
}


static DECLARE_THREADROUTINE(pulsDrv1Run){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) ctx->Thread->getArg();
	int cnt = 1000;
	float a = 0.f;
	while(cnt--){
		pwmDrv->setDuty(pwmDrv,1,cnt / 1000.f);
		ctx->Thread->yield(1);
	}
	cnt = 0;
	return tchOK;
}

static DECLARE_THREADROUTINE(pulsDrv2Run){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) ctx->Thread->getArg();
	int cnt = 1000;
	while(cnt--){
		pwmDrv->setDuty(pwmDrv,1,cnt / 1000.f);
		ctx->Thread->yield(1);
	}
	cnt = 0;
	do{
		fvs[cnt] = cnt * 0.001f;
	}while(cnt++ < 1000);

	pwmDrv->write(pwmDrv,1,fvs,1000);
	return tchOK;
}


static DECLARE_THREADROUTINE(pulseGenRun){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) ctx->Thread->getArg();
	pwmDrv->setDuty(pwmDrv,2,0.5);
	pwmDrv->write(pwmDrv,0,fvs,1000);
	return tchOK;
}

static DECLARE_THREADROUTINE(pulseCon1Run){
	tch_tcaptHandle* capt = (tch_tcaptHandle*) ctx->Thread->getArg();
	capt->read(capt,0,ffvs,1000,tchWaitForever);
	return tchOK;
}

static DECLARE_THREADROUTINE(pulseCon2Run){
	tch_tcaptHandle* capt = (tch_tcaptHandle*) ctx->Thread->getArg();
	capt->read(capt,2,ffvs1,1000,tchWaitForever);
	return tchOK;
}

