/*
 * led_blinker.c
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */

#include "tch.h"
#include "led_blinker.h"


static float sin[] = {0.5314f,0.5627f,0.5937f,0.6244f,0.6545f,0.6841f,0.7129f,0.7409f,0.7679f,
		0.7939f,0.8187f,0.8423f,0.8645f,0.8853f,0.9045f,0.9222f,0.9382f,0.9524f,0.9649f,
		0.9756f,0.9843f,0.9912f,0.9961f,0.9990f,1.0000f,0.9990f,0.9961f,0.9912f,0.9843f,
		0.9756f,0.9649f,0.9524f,0.9382f,0.9222f,0.9045f,0.8853f,0.8645f,0.8423f,0.8187f,
		0.7939f,0.7679f,0.7409f,0.7129f,0.6841f,0.6545f,0.6244f,0.5937f,0.5627f,0.5314f,
		0.5000f,0.4686f,0.4374f,0.4063f,0.3757f,0.3455f,0.3160f,0.2871f,0.2591f,0.2321f,
		0.2061f,0.1813f,0.1578f,0.1355f,0.1148f,0.0955f,0.0779f,0.0619f,0.0476f,0.0351f,
		0.0245f,0.0157f,0.0089f,0.0040f,0.0010f,0.0000f,0.0010f,0.0040f,0.0089f,0.0157f,
		0.0245f,0.0351f,0.0476f,0.0619f,0.0779f,0.0955f,0.1148f,0.1355f,0.1578f,0.1813f,
		0.2061f,0.2321f,0.2591f,0.2871f,0.3160f,0.3455f,0.3757f,0.4063f,0.4374f,0.4686f,0.5000f};

static tch_threadId shifter1;
static tch_threadId shifter2;

static DECLARE_THREADROUTINE(led_shifter1);
static DECLARE_THREADROUTINE(led_shifter2);


tchStatus start_led_blink(const tch_core_api_t* api)
{
	thread_config_t thread_cfg;
	api->Thread->initConfig(&thread_cfg,led_shifter1,Normal,0,0,"ledshifter1");
	shifter1 = api->Thread->create(&thread_cfg, NULL);

	api->Thread->initConfig(&thread_cfg, led_shifter2,Normal,0,0,"ledshifter2");
	shifter2 = api->Thread->create(&thread_cfg, NULL);

	api->Thread->start(shifter1);
	api->Thread->start(shifter2);
	return tchOK;
}

tchStatus exit_led_blink(const tch_core_api_t* api)
{
	api->Thread->exit(shifter1,tchOK);
	api->Thread->exit(shifter2,tchOK);
	return tchOK;
}



static DECLARE_THREADROUTINE(led_shifter1)
{
	tch_hal_module_gpio_t* gpio = (tch_hal_module_gpio_t*) ctx->Module->request(MODULE_TYPE_GPIO);
	gpio_config_t iocfg;
	gpio->initCfg(&iocfg);
	iocfg.Mode = GPIO_Mode_OUT;
	iocfg.Otype = GPIO_Otype_PP;
	iocfg.PuPd = GPIO_PuPd_Float;
	iocfg.popt = ActOnSleep;
	tch_gpioHandle* handle = gpio->allocIo(ctx, tch_gpio5, (1 << 6 | 1 << 7),
			&iocfg, tchWaitForever);
	tch_bState led = bSet;
	uint32_t cnt = 0;
	while (TRUE) {
		handle->out(handle, 1 << 6, led);
		ctx->Thread->yield(200);
		handle->out(handle, 1 << 7, led);
		led = !led;
		print_dbg("Led Shifted  %d times\n\r",cnt++);
		ctx->Thread->yield(200);
	}
	return tchOK;
}

DECLARE_THREADROUTINE(led_shifter2)
{

	tch_hal_module_timer* timer = (tch_hal_module_timer*) ctx->Module->request(MODULE_TYPE_TIMER);
	pwm_config_t pcfg;
	pcfg.UnitTime = TIMER_UNITTIME_uSEC;
	pcfg.PeriodInUnitTime = 1000;
	pcfg.pwrOpt = ActOnSleep;
	tch_pwmHandle* pwm1 = timer->openPWM(ctx, tch_TIMER8, &pcfg, tchWaitForever);
	tch_pwmHandle* pwm2 = timer->openPWM(ctx, tch_TIMER9, &pcfg, tchWaitForever);


	pwm1->setDuty(pwm1,0,0.1f);
	pwm1->setOutputEnable(pwm1,0,TRUE,tchWaitForever);
	pwm1->start(pwm1);

	pwm2->setDuty(pwm2,0,0.1f);
	pwm2->setOutputEnable(pwm2,0,TRUE,tchWaitForever);
	pwm2->start(pwm2);

	char p1i = 0;
	char p2i = 25;

	while(TRUE)
	{
		pwm1->setDuty(pwm1,0,sin[p1i++]);
		pwm2->setDuty(pwm2,0,sin[p2i++]);

		if(p1i == 99)
			p1i = 0;
		if(p2i == 99)
			p2i = 0;
		ctx->Thread->yield(20);

	}
	return tchOK;
}
