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
static DECLARE_THREADROUTINE(pulseConRun);
static float fvs[1000];
static float ffvs[1000];

tchStatus timer_performTest(tch* env){

	tchStatus result = osOK;

	// ************* Start of test for Basic Timer Function ********************** //
	tch_gptimerDef gptDef;
	gptDef.UnitTime = env->Device->timer->UnitTime.mSec;
	gptDef.pwrOpt = ActOnSleep;

	tch_gptimerHandle* gptimer = env->Device->timer->openGpTimer(env,env->Device->timer->timer.timer0,&gptDef,osWaitForever);

	tch_threadId waiterThread1;
	tch_threadId waiterThread2;

	uint8_t* waiterThread1Stk = (uint8_t*) env->Mem->alloc(1 << 9);
	uint8_t* waiterThread2Stk = (uint8_t*) env->Mem->alloc(1 << 9);

	tch_threadCfg thcfg;
	thcfg._t_name = "Waiter1";
	thcfg._t_routine = waiter1Run;
	thcfg._t_stack = waiterThread1Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;


	waiterThread1 = env->Thread->create(&thcfg,gptimer);


	thcfg._t_name = "Waiter2";
	thcfg._t_routine = waiter2Run;
	thcfg._t_stack = waiterThread2Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	waiterThread2 = env->Thread->create(&thcfg,gptimer);


	env->Thread->start(waiterThread1);
	env->Thread->start(waiterThread2);

	result = env->Thread->join(waiterThread1,osWaitForever);
	result = env->Thread->join(waiterThread2,osWaitForever);

	gptimer->close(gptimer);   // gptimer test complete

	// ************* End of test for Basic Timer Function ********************** //


	if(result != osOK)
		return osErrorOS;


	// ************* Start of test for PWM Out Function ********************** //

	tch_pwmDef pwmDef;
	pwmDef.PeriodInUnitTime = 1000;
	pwmDef.UnitTime = env->Device->timer->UnitTime.uSec;
	pwmDef.pwrOpt = ActOnSleep;
	int cnt = 100000;
	tch_pwmHandle* pwmDrv = NULL;
	while(cnt--){
		pwmDrv = env->Device->timer->openPWM(env,env->Device->timer->timer.timer0,&pwmDef,osWaitForever);
		if(pwmDrv)
			pwmDrv->close(pwmDrv);
	}

	pwmDrv = env->Device->timer->openPWM(env,env->Device->timer->timer.timer0,&pwmDef,osWaitForever);
	thcfg._t_name = "PulseDrv1";
	thcfg._t_routine = pulsDrv1Run;
	thcfg._t_stack = waiterThread1Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	waiterThread1 = env->Thread->create(&thcfg,pwmDrv);



	thcfg._t_name = "pulseDrv2";
	thcfg._t_routine = pulsDrv2Run;
	thcfg._t_stack = waiterThread2Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;
	waiterThread2 = env->Thread->create(&thcfg,pwmDrv);

	env->Thread->start(waiterThread1);
	env->Thread->start(waiterThread2);

	result = env->Thread->join(waiterThread1,osWaitForever);
	result = env->Thread->join(waiterThread2,osWaitForever);




	pwmDrv->close(pwmDrv);


	// ************* End of test for PWM Out Function ********************** //

	pwmDef.PeriodInUnitTime = 1000;
	pwmDef.UnitTime = env->Device->timer->UnitTime.uSec;
	pwmDef.pwrOpt = ActOnSleep;

	pwmDrv = env->Device->timer->openPWM(env,env->Device->timer->timer.timer2,&pwmDef,osWaitForever);
	if(!pwmDrv)
		return osErrorOS;

	tch_tcaptDef captDef;
	captDef.Polarity = env->Device->timer->Polarity.positive;
	captDef.UnitTime = env->Device->timer->UnitTime.uSec;
	captDef.periodInUnitTime = 1000;
	captDef.pwrOpt = ActOnSleep;

	tch_tcaptHandle* capt = env->Device->timer->openTimerCapture(env,env->Device->timer->timer.timer0,&captDef,osWaitForever);
	if(!capt)
		return osErrorOS;

	thcfg._t_name = "Pgen";
	thcfg._t_routine = pulseGenRun;
	thcfg._t_stack = waiterThread1Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	tch_threadId pgenThread = env->Thread->create(&thcfg,pwmDrv);

	thcfg._t_name = "Pcon";
	thcfg._t_routine = pulseConRun;
	thcfg._t_stack = waiterThread2Stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 1 << 9;

	tch_threadId pconThread = env->Thread->create(&thcfg,capt);

	env->Thread->start(pgenThread);
	env->Thread->start(pconThread);

	env->Thread->join(pgenThread,osWaitForever);
	env->Thread->join(pconThread,osWaitForever);

	env->Mem->free(waiterThread1Stk);
	env->Mem->free(waiterThread2Stk);

	capt->close(capt);
	pwmDrv->close(pwmDrv);

	// ************* Start of test for PWM Input Function ****************** //



	return result;
}


static DECLARE_THREADROUTINE(waiter1Run){
	tch_gptimerHandle* gptimer = (tch_gptimerHandle*)sys->Thread->getArg();
	int cnt = 1000;
	while(cnt--)
		gptimer->wait(gptimer,10);
	return osOK;

}

static DECLARE_THREADROUTINE(waiter2Run){
	tch_gptimerHandle* gptimer = (tch_gptimerHandle*)sys->Thread->getArg();
	int cnt = 1000;
	while(cnt--)
		gptimer->wait(gptimer,10);
	return osOK;
}


static DECLARE_THREADROUTINE(pulsDrv1Run){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) sys->Thread->getArg();
	int cnt = 1000;
	float a = 0.f;
	while(cnt--){
		pwmDrv->setDuty(pwmDrv,1,cnt / 1000.f);
		sys->Thread->sleep(1);
	}
	cnt = 0;
	return osOK;
}

static DECLARE_THREADROUTINE(pulsDrv2Run){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) sys->Thread->getArg();
	int cnt = 1000;
	while(cnt--){
		pwmDrv->setDuty(pwmDrv,1,cnt / 1000.f);
		sys->Thread->sleep(1);
	}
	cnt = 0;
	do{
		fvs[cnt] = cnt * 0.001f;
	}while(cnt++ < 1000);

	pwmDrv->write(pwmDrv,1,fvs,1000);
	return osOK;
}


static DECLARE_THREADROUTINE(pulseGenRun){
	tch_pwmHandle* pwmDrv = (tch_pwmHandle*) sys->Thread->getArg();
	pwmDrv->write(pwmDrv,0,fvs,1000);
	return osOK;
}

static DECLARE_THREADROUTINE(pulseConRun){
	tch_tcaptHandle* capt = (tch_tcaptHandle*) sys->Thread->getArg();
	capt->read(capt,1,ffvs,1000,osWaitForever);
	return osOK;
}
