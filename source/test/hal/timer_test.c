/*
 * timer_test.c
 *
 *  Created on: 2014. 10. 13.
 *      Author: innocentevil
 */


#include "tch.h"
#include "timer_test.h"

tchStatus timer_performTest(tch* env){
	tch_gptimerDef gptDef;
	gptDef.UnitTime = env->Device->timer->UnitTime.mSec;
	gptDef.pwrOpt = ActOnSleep;

	tch_gptimerHandle* gptimer = env->Device->timer->openGpTimer(env,env->Device->timer->timer.timer0,&gptDef,osWaitForever);
	gptimer->wait(gptimer,1000);
	gptimer->wait(gptimer,1000);

	gptimer->close(gptimer);

	gptDef.UnitTime = env->Device->timer->UnitTime.mSec;
	gptimer = env->Device->timer->openGpTimer(env,env->Device->timer->timer.timer0,&gptDef,osWaitForever);
	gptimer->wait(gptimer,2000);
	gptimer->wait(gptimer,2000);
	gptimer->wait(gptimer,2000);
	gptimer->wait(gptimer,2000);
	gptimer->close(gptimer);
	return osOK;

}


