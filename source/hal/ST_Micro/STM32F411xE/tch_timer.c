/*
 * tch_timer.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */

#include "tch_hal.h"
#include "tch_gpio.h"
#include "tch_timer.h"

#include "kernel/tch_interrupt.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_kmod.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/string.h"





/*     class identifier for checking validity of instance    */
#ifndef TIMER_GP_CLASS_KEY
#define TIMER_GP_CLASS_KEY       ((uint16_t) 0x6401)
#endif

#ifndef TIMER_PWM_CLASS_KEY
#define TIMER_PWM_CLASS_KEY      ((uint16_t) 0x6411)
#endif

#ifndef TIMER_CAPTURE_CLASS_KEY
#define TIMER_CAPTURE_CLASS_KEY  ((uint16_t) 0x6421)
#endif

#define TIMER_LP_ACTIVE    ((uint32_t) 0x10000)
#define TIMER_IO_CNT_Msk   ((uint32_t) 0xE0000)
#define TIMER_IO_CNT_Pos   ((uint8_t) 17)


#define tch_timer_GPtValidate(gpt_ins)           do{\
	((tch_gptimer_handle_proto*) gpt_ins)->status |= (((uint32_t)gpt_ins & 0xFFFF) ^ TIMER_GP_CLASS_KEY);\
}while(0)

#define tch_timer_GPtInvalidate(gpt_ins)        do{\
	((tch_gptimer_handle_proto*) gpt_ins)->status &= ~0xFFFF;\
}while(0)

#define tch_timer_GPtIsValid(gpt_ins)    ((((tch_gptimer_handle_proto*) gpt_ins)->status & 0xFFFF) == (((uint32_t)gpt_ins & 0xFFFF) ^ TIMER_GP_CLASS_KEY))

#define tch_timer_GPtSetLpActive(gpt_ins)        do{\
	((tch_gptimer_handle_proto*) gpt_ins)->status |= TIMER_LP_ACTIVE;\
}while(0)

#define tch_timer_GPtIsLpActive(gpt_ins)    (((tch_gptimer_handle_proto*) gpt_ins)->status & TIMER_LP_ACTIVE)

#define tch_timer_GPtClrLpActive(gpt_ins)        do{\
	((tch_gptimer_handle_proto*) gpt_ins)->status &= ~TIMER_LP_ACTIVE;\
}while(0)


#define tch_timer_PWMValidate(pwm_ins)           do{\
	((tch_pwm_handle_proto*) pwm_ins)->status |= (((uint32_t)pwm_ins & 0xFFFF) ^ TIMER_PWM_CLASS_KEY);\
}while(0)

#define tch_timer_PWMInvalidate(pwm_ins)        do{\
	((tch_pwm_handle_proto*) pwm_ins)->key &= 0xFFFF;\
}while(0)

#define tch_timer_PWMIsValid(pwm_ins)    ((((tch_pwm_handle_proto*) pwm_ins)->status & 0xFFFF) == (((uint32_t)pwm_ins & 0xFFFF) ^ TIMER_PWM_CLASS_KEY))

#define tch_timer_PWMSetLpActive(pwm_ins)        do{\
	((tch_pwm_handle_proto*) pwm_ins)->status |= TIMER_LP_ACTIVE;\
}while(0)

#define tch_timer_PWMIsLpActive(pwm_ins)    (((tch_pwm_handle_proto*) pwm_ins)->status & TIMER_LP_ACTIVE)

#define tch_timer_PWMClrLpActive(pwm_ins)        do{\
	((tch_pwm_handle_proto*) pwm_ins)->status &= ~TIMER_LP_ACTIVE;\
}while(0)



#define tch_timer_tCaptValidate(capt_ins)           do{\
	((tch_tcapt_handle_proto*) capt_ins)->status |= (((uint32_t)capt_ins & 0xFFFF) ^ TIMER_CAPTURE_CLASS_KEY);\
}while(0)

#define tch_timer_tCaptInvalidate(capt_ins)        do{\
	((tch_tcapt_handle_proto*) capt_ins)->status &= ~0xFFFF;\
}while(0)

#define tch_timer_tCaptIsValid(capt_ins)    ((((tch_tcapt_handle_proto*) capt_ins)->status & 0xFFFF) == (((uint32_t)capt_ins & 0xFFFF) ^ TIMER_CAPTURE_CLASS_KEY))

#define tch_timer_tCaptSetLpActive(capt_ins)        do{\
	((tch_tcapt_handle_proto*) capt_ins)->status |= TIMER_LP_ACTIVE;\
}while(0)

#define tch_timer_tCaptIsLpActive(capt_ins)    (((tch_tcapt_handle_proto*) capt_ins)->status & TIMER_LP_ACTIVE)

#define tch_timer_tCaptClrLpActive(capt_ins)        do{\
	((tch_tcapt_handle_proto*) capt_ins)->status &= ~TIMER_LP_ACTIVE;\
}while(0)



struct tch_gptimer_req_t {
	uint32_t utick;
	void* ins;
};

typedef struct tch_gptimer_handle_proto_t {
	tch_gptimerHandle     _pix;
	uint32_t               status;
	tch_timer              timer;
	const tch_core_api_t*             env;
	tch_msgqId*            msgqs;
}tch_gptimer_handle_proto;

typedef struct tch_pwm_handle_proto_t{
	tch_pwmHandle         _pix;
	uint32_t               status;
	tch_timer              timer;
	const tch_core_api_t*             env;
	tch_gpioHandle**       iohandle;
	tch_waitqId            uev_wq;
	tch_semId              uev_sem;
	tch_mtxId              mtx;
}tch_pwm_handle_proto;

typedef struct tch_tcapt_handle_proto_t{
	tch_tcaptHandle       _pix;
	uint32_t               status;
	tch_timer              timer;
	const tch_core_api_t*             env;
	tch_gpioHandle**       iohandle;
	tch_msgqId*            msgqs;
	tch_mtxId              mtx;
	tch_condvId            condv;
}tch_tcapt_handle_proto;


///////            Timer Manager Function               ///////
__USER_API__ static tch_gptimerHandle* tch_timer_allocGptimerUnit(const tch_core_api_t* sys,tch_timer timer,gptimer_config_t* cfg,uint32_t timeout);
__USER_API__ static tch_pwmHandle* tch_timer_allocPWMUnit(const tch_core_api_t* sys,tch_timer timer,pwm_config_t* cfg,uint32_t timeout);
__USER_API__ static tch_tcaptHandle* tch_timer_allocCaptureUnit(const tch_core_api_t* sys,tch_timer timer,pcapt_config_t* cfg,uint32_t timeout);
__USER_API__ static uint32_t tch_timer_getChannelCount(tch_timer timer);
__USER_API__ static uint8_t tch_timer_getPrecision(tch_timer timer);


//////         General Purpose Timer Function           ///////
__USER_API__ static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t utick);
__USER_API__ static tchStatus tch_gptimer_close(tch_gptimerHandle* self);
__USER_API__ static BOOL tch_timer_handle_gptInterrupt(tch_gptimer_handle_proto* ins,tch_timer_descriptor* desc);
__USER_API__ static BOOL tch_timer_handle_pwmInterrupt(tch_pwm_handle_proto* ins,tch_timer_descriptor* desc);
__USER_API__ static BOOL tch_timer_handle_tcaptInterrupt(tch_tcapt_handle_proto* ins, tch_timer_descriptor* desc);


//////            PWM fucntion                        //////
__USER_API__ static BOOL tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch,float duty);
__USER_API__ static tchStatus tch_pwm_write(tch_pwmHandle* self,uint32_t ch,float* fduty,size_t sz);
__USER_API__ static tchStatus tch_pwm_setOutputEnable(tch_pwmHandle* self,uint8_t ch,BOOL enable,uint32_t timeout);
__USER_API__ static float tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch);
__USER_API__ static tchStatus tch_pwm_close(tch_pwmHandle* self);
__USER_API__ static tchStatus tch_pwm_start(tch_pwmHandle* self);
__USER_API__ static tchStatus tch_pwm_stop(tch_pwmHandle* self);


/////             Pulse Capture Function                /////
__USER_API__ static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout);
__USER_API__ static tchStatus tch_tcapt_setInputEnable(tch_tcaptHandle* self,uint8_t ch,BOOL enable,uint32_t timeout);
__USER_API__ static tchStatus tch_tcapt_close(tch_tcaptHandle* self);


static int tch_timer_init(void);
static void tch_timer_exit(void);
static void tch_timer_handleInterrupt(tch_timer timer);



__USER_RODATA__ tch_hal_module_timer TIMER_Ops = {
		.count = MFEATURE_TIMER,
		.openGpTimer = tch_timer_allocGptimerUnit,
		.openPWM = tch_timer_allocPWMUnit,
		.openTimerCapture = tch_timer_allocCaptureUnit,
		.getChannelCount = tch_timer_getChannelCount,
		.getPrecision = tch_timer_getPrecision
};

static tch_mtxCb	lock;
static tch_condvCb  condv;


static int tch_timer_init(void)
{
	tch_mutexInit(&lock);
	tch_condvInit(&condv);
	return tch_kmod_register(MODULE_TYPE_TIMER,TIMER_GP_CLASS_KEY,&TIMER_Ops,FALSE);
}

static void tch_timer_exit(void)
{
	tch_kmod_unregister(MODULE_TYPE_TIMER,TIMER_GP_CLASS_KEY);
	tch_mutexDeinit(&lock);
	tch_condvDeinit(&condv);
}

MODULE_INIT(tch_timer_init);
MODULE_EXIT(tch_timer_exit);


///////            Timer Manager Function               ///////
__USER_API__ static tch_gptimerHandle* tch_timer_allocGptimerUnit(const tch_core_api_t* env,tch_timer timer,gptimer_config_t* gpt_def,uint32_t timeout)
{
	uint16_t tmpccer = 0, tmpccmr = 0;
	tch_gptimer_handle_proto* ins = NULL;

	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];
	if(env->Mtx->lock(&lock,timeout) != tchOK)
	{
		return NULL;
	}
	while(timDesc->_handle)
	{
		if(env->Condv->wait(&condv,&lock,timeout) != tchOK)
		{
			return NULL;
		}
	}

	timDesc->_handle = ins = env->Mem->alloc(sizeof(tch_gptimer_handle_proto));
	env->Mtx->unlock(&lock);

	mset(ins,0,sizeof(tch_gptimer_handle_proto));

	/* bind instance method and internal member var  */
	ins->_pix.close = tch_gptimer_close;
	ins->_pix.wait = tch_gptimer_wait;
	ins->msgqs = env->Mem->alloc(timDesc->channelCnt * sizeof(tch_msgqId));

	ins->env = env;
	ins->timer = timer;
	tch_timer_GPtValidate(ins);

	TIM_TypeDef* timerHw = (TIM_TypeDef*)timDesc->_hw;

	//enable clock
	*timDesc->_clkenr |= timDesc->clkmsk;
	if(gpt_def->pwrOpt == ActOnSleep)
	{
		*timDesc->_lpclkenr |= timDesc->lpclkmsk;
		tch_timer_GPtSetLpActive(ins);
	}
	else
	{
		*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
		tch_timer_GPtClrLpActive(ins);
	}

	*timDesc->rstr |= timDesc->rstmsk;
	*timDesc->rstr &= ~timDesc->rstmsk;


	uint32_t psc = 1;
	if(timDesc->_clkenr == &RCC->APB1ENR)
		psc = 2;

	switch(gpt_def->UnitTime){
	case TIMER_UNITTIME_mSEC:
		timerHw->PSC = SYS_CLK / psc / 1000 - 1;
		break;
	case TIMER_UNITTIME_nSEC:   // out of capability
	case TIMER_UNITTIME_uSEC:
		timerHw->PSC = SYS_CLK / psc / 1000000 - 1;
	}


	timerHw->EGR |= TIM_EGR_UG;


	if(timDesc->channelCnt > 0)
	{   // if requested timer h/w supprot channel 1 initialize its related registers

		timerHw->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);  // clear ccer output enable
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;


		tmpccer &= ~(TIM_CCER_CC1NP | TIM_CCER_CC1P);
		tmpccmr &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);   //* set capture/compare as frozen mode and configured as output
		if((timerHw == TIM1) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS1 | TIM_CR2_OIS1N);
		}
		ins->msgqs[0] = env->MsgQ->create(1);
		timerHw->CCER = tmpccer;
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCR1 = 0;
	}

	if(timDesc->channelCnt > 1)
	{   // same as above for channel 2
		timerHw->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;

		tmpccer &= ~(TIM_CCER_CC2NP | TIM_CCER_CC2P);
		tmpccmr &= ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
		if((timerHw == TIM1) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS2 | TIM_CR2_OIS2N);
		}
		ins->msgqs[1] = env->MsgQ->create(1);
		timerHw->CCER = tmpccer;
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCR2 = 0;
	}
	if(timDesc->channelCnt > 2)
	{   //.. for channel 3
		timerHw->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC3NP | TIM_CCER_CC3P);
		tmpccmr &= ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M);
		if((timerHw == TIM1) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS3 | TIM_CR2_OIS3N);
		}

		ins->msgqs[2] = env->MsgQ->create(1);
		timerHw->CCER = tmpccer;
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCR3 = 0;
	}
	if(timDesc->channelCnt > 3)
	{   // .. for channel 4
		timerHw->CCER &= ~TIM_CCER_CC4E;
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC4NP | TIM_CCER_CC4P);
		tmpccmr &= ~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M);
		if((timerHw == TIM1) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS4);
		}
		ins->msgqs[3] = env->MsgQ->create(1);
		timerHw->CCER = tmpccer;
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCR4 = 0;
	}

	timerHw->ARR = 0xFFFF;
	if(timDesc->precision == 32)
		timerHw->ARR = 0xFFFFFFFF;
	timerHw->CR1 |= TIM_CR1_CEN;
	// TODO : dummy handler ,consider supporting isr remap
	tch_enableInterrupt(timDesc->irq,PRIORITY_2,NULL);
	return (tch_gptimerHandle*) ins;
}

__USER_API__ static tch_pwmHandle* tch_timer_allocPWMUnit(const tch_core_api_t* env,tch_timer timer,pwm_config_t* tdef,uint32_t timeout)
{
	uint16_t tmpccer = 0, tmpccmr = 0;
	tch_pwm_handle_proto* ins = NULL;
	const tch_timer_bs_t* timBcfg = &TIMER_BD_CFGs[timer];
	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];

	if(env->Mtx->lock(&lock,timeout) != tchOK)
	{
		env->Mem->free(ins);
		return NULL;
	}

	while(timDesc->_handle)
	{
		if(env->Condv->wait(&condv,&lock,timeout) != tchOK)
		{
			env->Mem->free(ins);
			return NULL;
		}
	}
	timDesc->_handle = ins = (tch_pwm_handle_proto*) env->Mem->alloc(sizeof(tch_pwm_handle_proto));
	env->Mtx->unlock(&lock);

	mset(ins,0,sizeof(tch_pwm_handle_proto));

	ins->_pix.getDuty = tch_pwm_getDuty;
	ins->_pix.setDuty = tch_pwm_setDuty;
	ins->_pix.write = tch_pwm_write;
	ins->_pix.close = tch_pwm_close;
	ins->_pix.start = tch_pwm_start;
	ins->_pix.stop = tch_pwm_stop;
	ins->_pix.setOutputEnable = tch_pwm_setOutputEnable;
	ins->timer = timer;
	ins->env = env;
	//ins->uev_rndv = env->Rendezvous->create();
	ins->uev_wq = env->WaitQ->create(WAITQ_POL_FIFO);
	// TODO : replace barrier
	ins->uev_sem = env->Sem->create(timDesc->channelCnt);
	ins->mtx = env->Mtx->create();


	tch_timer_PWMValidate(ins);
	ins->iohandle = env->Mem->alloc(sizeof(tch_gpioHandle*) * timDesc->channelCnt);
	mset(ins->iohandle,0,(sizeof(tch_gpioHandle*) * timDesc->channelCnt));

	TIM_TypeDef* timerHw = (TIM_TypeDef*)timDesc->_hw;

	//enable clock
	*timDesc->_clkenr |= timDesc->clkmsk;
	if(tdef->pwrOpt == ActOnSleep)
	{
		*timDesc->_lpclkenr |= timDesc->lpclkmsk;
		tch_timer_PWMSetLpActive(ins);
	}
	else
	{
		*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
		tch_timer_PWMClrLpActive(ins);
	}

	*timDesc->rstr |= timDesc->rstmsk;
	*timDesc->rstr &= ~timDesc->rstmsk;

	uint8_t chidx = 0;
	uint32_t validIoCnt = 0;
	while(chidx < timDesc->channelCnt)
	{
		if(timBcfg->chp[chidx++] != -1)
			validIoCnt++;
	}
	ins->status &= ~TIMER_IO_CNT_Msk;
	ins->status |= (validIoCnt << TIMER_IO_CNT_Pos);


	uint32_t psc = 1;
	if(timDesc->_clkenr == &RCC->APB1ENR)
		psc = 2;

	switch(tdef->UnitTime)
	{
	case TIMER_UNITTIME_mSEC:
		timerHw->PSC = (SYS_CLK / psc / 1000) - 1;
		break;
	case TIMER_UNITTIME_nSEC:   // out of capability
	case TIMER_UNITTIME_uSEC:
		timerHw->PSC = (SYS_CLK / psc / 1000000) - 1;
	}

	timerHw->CR1 |= TIM_CR1_ARPE;// set auto preload enable @ CR1 (Set ARPE bit)

	if(timDesc->channelCnt > 0)
	{   // if requested timer h/w supprot channel 1 initialize its related registers

		timerHw->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);  // clear ccer output enable
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;


		tmpccer &= ~(TIM_CCER_CC1NP | TIM_CCER_CC1P);
		tmpccmr &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);   //* set capture/compare as frozen mode and configured as output
		tmpccmr |= ((6 << 4) | TIM_CCMR1_OC1PE);         // set to pwm mode @ CCMRx (OCxM / OCxPE)
		tmpccer |= TIM_CCER_CC1E;                      // set polarity of output(Active High) and enable it  @ CCER (CCxP / CCxE)

		if((timerHw == TIM2) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS1 | TIM_CR2_OIS1N);
		}
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCER = tmpccer;
		timerHw->CCR1 = 0;
	}

	if(timDesc->channelCnt > 1)
	{   // same as above for channel 2
		timerHw->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;

		tmpccer &= ~(TIM_CCER_CC2NP | TIM_CCER_CC2P);
		tmpccmr &= ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
		tmpccmr |= ((6 << 12) | TIM_CCMR1_OC2PE);
		tmpccer |= TIM_CCER_CC2E;

		if((timerHw == TIM2) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS2 | TIM_CR2_OIS2N);
		}
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCER = tmpccer;
		timerHw->CCR2 = 0;
	}

	if(timDesc->channelCnt > 2)
	{   //.. for channel 3
		timerHw->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC3NP | TIM_CCER_CC3P);
		tmpccmr &= ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M);
		tmpccmr |= ((6 << 4) | TIM_CCMR2_OC3PE);
		tmpccer |= TIM_CCER_CC3E;

		if((timerHw == TIM2) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS3 | TIM_CR2_OIS3N);
		}
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCER = tmpccer;
		timerHw->CCR3 = 0;
	}

	if(timDesc->channelCnt > 3)
	{   // .. for channel 4
		timerHw->CCER &= ~TIM_CCER_CC4E;
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC4NP | TIM_CCER_CC4P);
		tmpccmr &= ~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M);
		tmpccmr |= ((6 << 12) | TIM_CCMR2_OC4PE);
		tmpccer |= TIM_CCER_CC4E;

		if((timerHw == TIM2) || (timerHw == TIM8))
		{
			timerHw->CR2 &= ~(TIM_CR2_OIS4);
		}
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCER = tmpccer;
		timerHw->CCR4 = 0;
	}

	// TODO : dummy handler, consider supporting isr remap
	tch_enableInterrupt(timDesc->irq,PRIORITY_2, NULL);
	timerHw->ARR = tdef->PeriodInUnitTime;
	timerHw->EGR |= TIM_EGR_UG;

	return (tch_pwmHandle*) ins;

}

/*!
 *
 */
__USER_API__ static tch_tcaptHandle* tch_timer_allocCaptureUnit(const tch_core_api_t* env,tch_timer timer,pcapt_config_t* tdef,uint32_t timeout)
{
	tchStatus result = tchOK;
	uint16_t pmsk = 0;
	tch_tcapt_handle_proto* ins = (tch_tcapt_handle_proto*) env->Mem->alloc(sizeof(tch_tcapt_handle_proto));
	mset(ins,0,sizeof(tch_tcapt_handle_proto));

	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];
	const tch_timer_bs_t* tbs = &TIMER_BD_CFGs[timer];
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;

	if((tbs->chp[0] != -1) && (timDesc->channelCnt > 1))
	{
		pmsk |= (1 << tbs->chp[0]);
	}

	if((tbs->chp[2] != -1) && (timDesc->channelCnt > 3))
	{
		pmsk |= (1 << tbs->chp[2]);
	}

	if(!pmsk)
	{
		env->Mem->free(ins);
		return NULL;
	}

	ins->iohandle = env->Mem->alloc(sizeof(tch_gpioHandle*) * (timDesc->channelCnt / 2));

	if(!ins->iohandle)
	{
		env->Mem->free(ins);
		return NULL;
	}

	if((result = env->Mtx->lock(&lock,timeout)) != tchOK)
	{
		env->Mem->free(ins);
		return NULL;
	}

	while(timDesc->_handle)
	{
		if((result = env->Condv->wait(&condv,&lock,timeout)) != tchOK)
		{
			env->Mem->free(ins);
			return NULL;
		}
	}
	timDesc->_handle = ins;
	timDesc->ch_occp = 0;
	env->Mtx->unlock(&lock);

	ins->_pix.close = tch_tcapt_close;
	ins->_pix.read = tch_tcapt_read;
	ins->_pix.setInputEnable = tch_tcapt_setInputEnable;
	ins->env = env;

	ins->msgqs = (tch_msgqId*) env->Mem->alloc(sizeof(tch_msgqId) * 2);
	ins->msgqs[0] = env->MsgQ->create(1);
	ins->msgqs[1] = env->MsgQ->create(1);

	ins->mtx = env->Mtx->create();
	ins->condv = env->Condv->create();
	ins->timer = timer;
	tch_timer_tCaptValidate(ins);

	*timDesc->_clkenr |= timDesc->clkmsk;
	if(tdef->pwrOpt == ActOnSleep)
	{
		*timDesc->_lpclkenr |= timDesc->lpclkmsk;
		tch_timer_tCaptSetLpActive(ins);
	}
	else
	{
		*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
		tch_timer_tCaptClrLpActive(ins);
	}

	*timDesc->rstr |= timDesc->rstmsk;
	*timDesc->rstr &= ~timDesc->rstmsk;

	uint32_t psc = 1;
	if(timDesc->_clkenr == &RCC->APB1ENR)
	{
		psc = 2;
	}
	switch(tdef->UnitTime)
	{
	case TIMER_UNITTIME_mSEC:
		timerHw->PSC = (SYS_CLK / psc / 1000) - 1;
		break;
	case TIMER_UNITTIME_nSEC:   // out of capability
	case TIMER_UNITTIME_uSEC:
		timerHw->PSC = (SYS_CLK / psc / 1000000) - 1;
	}

	timerHw->SMCR &= ~(TIM_SMCR_TS | TIM_SMCR_SMS);
	timerHw->SMCR |= (TIM_SMCR_TS_2 | TIM_SMCR_TS_0 | TIM_SMCR_SMS_2);
	if(timDesc->channelCnt > 1)
	{   // if requested timer h/w supprot channel 1 initialize its related registers
		timerHw->CCR1 = 0;
		timerHw->CCR2 = 0;
		timerHw->CCMR1 &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_CC2S);
		timerHw->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP | TIM_CCER_CC2P | TIM_CCER_CC2NP | TIM_CCER_CC1E | TIM_CCER_CC2E);

		timerHw->CCMR1 |= (TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_1 | TIM_CCMR1_IC1F_0);
		if(tdef->Polarity == TIMER_POLARITY_NEGATIVE)
		{
			timerHw->CCER |= TIM_CCER_CC2P;
		}
		else
		{
			timerHw->CCER |= TIM_CCER_CC1P;
		}

		timerHw->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC2E);
		timerHw->DIER |= TIM_DIER_CC1IE;
	}

	if(timDesc->channelCnt > 3)
	{   // same as above for channel 2
		timerHw->CCR3 = 0;
		timerHw->CCR4 = 0;
		timerHw->CCMR2 &= ~(TIM_CCMR2_CC3S | TIM_CCMR2_CC4S);
		timerHw->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC3NP | TIM_CCER_CC4P | TIM_CCER_CC4NP | TIM_CCER_CC3E | TIM_CCER_CC4E);

		timerHw->CCMR2 |= (TIM_CCMR2_CC3S_0 | TIM_CCMR2_CC4S_1 | TIM_CCMR2_IC3F_0);
		if(tdef->Polarity == TIMER_POLARITY_NEGATIVE)
		{
			timerHw->CCER |= TIM_CCER_CC4P;
		}
		else
		{
			timerHw->CCER |= TIM_CCER_CC3P;
		}

		timerHw->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC4E);
		timerHw->DIER |= TIM_DIER_CC3IE;
	}
	// TODO : dummy handler, consider again when isr remap support added
	tch_enableInterrupt(timDesc->irq,PRIORITY_2, NULL);
	timerHw->CNT = 0;
	timerHw->CR1 = TIM_CR1_CEN;
	return (tch_tcaptHandle*) ins;
}

__USER_API__ static uint32_t tch_timer_getChannelCount(tch_timer timer)
{
	return TIMER_HWs[timer].channelCnt;
}

__USER_API__ static uint8_t tch_timer_getPrecision(tch_timer timer)
{
	return TIMER_HWs[timer].precision;
}

__USER_API__ static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t utick)
{
	tch_gptimer_handle_proto* ins = (tch_gptimer_handle_proto*) self;
	if(!self)
	{
		return FALSE;
	}

	if(!tch_timer_GPtIsValid(ins))
	{
		return FALSE;
	}

	tch_timer_descriptor* thw = &TIMER_HWs[ins->timer];
	TIM_TypeDef* timerHw = thw->_hw;
	int id = 0;
	uint8_t chmsk = thw->ch_occp;
	const tch_core_api_t* env = ins->env;
	struct tch_gptimer_req_t req;
	req.ins = self;
	req.utick = utick;
	do
	{
		if(!(chmsk & 1))
		{
			thw->ch_occp |= (1 << id); // set occp bit
			switch(id)
			{
			case 0:
				timerHw->CCR1 = timerHw->CNT + utick;
				timerHw->DIER |= TIM_DIER_CC1IE;
				break;
			case 1:
				timerHw->CCR2 = timerHw->CNT + utick;
				timerHw->DIER |= TIM_DIER_CC2IE;
				break;
			case 2:
				timerHw->CCR3 = timerHw->CNT + utick;
				timerHw->DIER |= TIM_DIER_CC3IE;
				break;
			case 3:
				timerHw->CCR4 = timerHw->CNT + utick;
				timerHw->DIER |= TIM_DIER_CC4IE;
				break;
			}
			tchEvent evt = env->MsgQ->get(ins->msgqs[id],tchWaitForever);
			if(evt.status != tchEventMessage)
			{
				return FALSE;
			}
			thw->ch_occp &= ~(1 << id);
			return TRUE;
		}
		chmsk >>= 1;
	} while(id++ < thw->channelCnt);
	return FALSE;   // couldn't find available timer channel ( all occupied)
}

__USER_API__ static tchStatus tch_gptimer_close(tch_gptimerHandle* self){
	tch_gptimer_handle_proto* ins = (tch_gptimer_handle_proto*) self;
	int chIdx = 0;
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_GPtIsValid(ins))
	{
		return tchErrorResource;
	}

	tch_timer_descriptor* timDesc = &TIMER_HWs[ins->timer];
	const tch_core_api_t* env = ins->env;
	tchStatus result = tchOK;
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;
	timerHw->CR1 = 0;                // disable timer count
	timerHw->SR = 0;
	timerHw->CNT = 0;
	timerHw->DIER = 0;
	if((result = env->Mtx->lock(&lock,tchWaitForever)) != tchOK)
	{
		return result;
	}

	*timDesc->_clkenr &= ~timDesc->clkmsk;
	*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
	timDesc->_handle = NULL;
	tch_disableInterrupt(timDesc->irq);

	env->Condv->wakeAll(&condv);
	env->Mtx->unlock(&lock);


	while(chIdx < timDesc->channelCnt)
	{
		env->MsgQ->destroy(ins->msgqs[chIdx++]);
	}
	env->Mem->free(ins->msgqs);
	env->Mem->free(ins);
	return tchOK;
}


//////            PWM fucntion                        //////

__USER_API__ static BOOL tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch,float duty)
{
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	TIM_TypeDef* timerHw = (TIM_TypeDef*)TIMER_HWs[ins->timer]._hw;
	if(!self)
	{
		return FALSE;
	}
	if(!tch_timer_PWMIsValid(ins))
	{
		return FALSE;
	}
	if(!(ch < TIMER_HWs[ins->timer].channelCnt))
	{
		return FALSE;
	}

	uint32_t dutyd = timerHw->ARR;
	dutyd = (uint32_t) ((float) dutyd * duty);
	switch(ch)
	{
	case 0:
		timerHw->CCR1 = dutyd;
		return TRUE;
	case 1:
		timerHw->CCR2 = dutyd;
		return TRUE;
	case 2:
		timerHw->CCR3 = dutyd;
		return TRUE;
	case 3:
		timerHw->CCR4 = dutyd;
		return TRUE;
	}
	return FALSE;
}

__USER_API__ static tchStatus tch_pwm_write(tch_pwmHandle* self,uint32_t ch,float* fduty,size_t sz){
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	tch_timer_descriptor* timDesc = (tch_timer_descriptor*) &TIMER_HWs[ins->timer];
	tchStatus result = tchOK;
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(ins))
	{
		return tchErrorParameter;
	}
	if(!(ch < TIMER_HWs[ins->timer].channelCnt))
	{
		return tchErrorParameter;
	}
	TIM_TypeDef* TimerHw = (TIM_TypeDef*)timDesc->_hw;
	volatile uint32_t* ccr = NULL;
	switch(ch)
	{
	case 0:
		ccr = &TimerHw->CCR1;
		break;
	case 1:
		ccr = &TimerHw->CCR2;
		break;
	case 2:
		ccr = &TimerHw->CCR3;
		break;
	case 3:
		ccr = &TimerHw->CCR4;
		break;
	}
	if((result = ins->env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
	{
		return result;
	}
	TimerHw->DIER |= TIM_DIER_UIE;    // enable update
	uint32_t dutyd = TimerHw->ARR;
	while(sz--)
	{
		*ccr = (uint32_t) (*(fduty++) * dutyd);
		if(sz)
		{
			//result = ins->env->Rendezvous->sleep(ins->uev_rndv,tchWaitForever);
			result = ins->env->WaitQ->sleep(ins->uev_wq, tchWaitForever);
			// TODO : replace barrier
		}
	}
	TimerHw->DIER &= ~TIM_DIER_UIE;
	result = ins->env->Mtx->unlock(ins->mtx);
	return result;
}


__USER_API__ static tchStatus tch_pwm_setOutputEnable(tch_pwmHandle* self,uint8_t ch,BOOL enable,uint32_t timeout)
{
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(self))
	{
		return tchErrorParameter;
	}
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	const tch_timer_bs_t* timBcfg = &TIMER_BD_CFGs[ins->timer];
	gpio_config_t iocfg;
	tch_hal_module_gpio_t* gpio = (tch_hal_module_gpio_t*) Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		return tchErrorResource;
	}
	if(enable && !ins->iohandle[ch] && (timBcfg->chp[ch] != -1))
	{
		gpio->initCfg(&iocfg);
		iocfg.Af = timBcfg->afv;
		iocfg.Mode = GPIO_Mode_AF;
		iocfg.popt = tch_timer_PWMIsLpActive(ins) ? ActOnSleep : NoActOnSleep;
		ins->iohandle[ch] = gpio->allocIo(ins->env,timBcfg->port,(1 << timBcfg->chp[ch]),&iocfg,timeout);
		return tchOK;
	}
	if(!enable && ins->iohandle[ch])
	{
		ins->iohandle[ch]->close(ins->iohandle[ch]);
		ins->iohandle[ch] = NULL;
		return tchOK;
	}
	return tchErrorParameter;
}


__USER_API__ static float tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch)
{
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	float tmparr, tmpccr = 0.f;
	if(!tch_timer_PWMIsValid(ins))
	{
		return 0.f;
	}

	TIM_TypeDef* timerHw = (TIM_TypeDef*) TIMER_HWs[ins->timer]._hw;
	tmparr = (float)timerHw->ARR;
	switch(ch)
	{
	case 0:
		tmpccr = (float)timerHw->CCR1;
		break;
	case 1:
		tmpccr = (float)timerHw->CCR2;
		break;
	case 2:
		tmpccr = (float)timerHw->CCR3;
		break;
	case 3:
		tmpccr = (float)timerHw->CCR4;
		break;
	}
	return tmpccr / tmparr;
}

__USER_API__ static tchStatus tch_pwm_start(tch_pwmHandle* self)
{
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	TIM_TypeDef* timHw = NULL;
	if(!ins)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(ins))
	{
		return tchErrorParameter;
	}
	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}
	timHw = (TIM_TypeDef*) TIMER_HWs[ins->timer]._hw;
	timHw->CR1 |= TIM_CR1_CEN;
	ins->env->Mtx->unlock(ins->mtx);
	return tchOK;
}

__USER_API__ static tchStatus tch_pwm_stop(tch_pwmHandle* self)
{
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	TIM_TypeDef* timHw = NULL;
	if(!ins)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(ins))
	{
		return tchErrorParameter;
	}
	if(ins->env->Mtx->lock(ins->mtx,tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}

	timHw = (TIM_TypeDef*) TIMER_HWs[ins->timer]._hw;
	timHw->CR1 &= ~TIM_CR1_CEN;
	ins->env->Mtx->unlock(ins->mtx);
	return tchOK;
}

__USER_API__ static tchStatus tch_pwm_close(tch_pwmHandle* self){
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(self))
	{
		return tchErrorParameter;
	}
	tchStatus result = tchOK;
	tch_pwm_handle_proto* ins = (tch_pwm_handle_proto*) self;
	if((result = ins->env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
	{
		return result;
	}
	tch_timer_descriptor* timDesc = &TIMER_HWs[ins->timer];
	const tch_core_api_t* env = ins->env;
	uint8_t chcnt = timDesc->channelCnt;
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;
	env->Mtx->destroy(ins->mtx);
	timerHw->CR1 &= ~TIM_CR1_CEN;                // disable timer count
	//env->Rendezvous->destroy(ins->uev_rndv);
	env->WaitQ->destroy(ins->uev_wq);
	// TODO: replace barrier
	env->Sem->destroy(ins->uev_sem);
	while(chcnt--)
	{
		if(ins->iohandle[chcnt])
		{	// free io resource
			ins->iohandle[chcnt]->close(ins->iohandle[chcnt]);
		}
	}

	env->Mem->free(ins->iohandle);
	if((result = env->Mtx->lock(&lock,tchWaitForever)) != tchOK)
	{
		return result;
	}
	*timDesc->_clkenr &= ~timDesc->clkmsk;
	*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
	tch_disableInterrupt(timDesc->irq);
	timDesc->_handle = NULL;
	env->Condv->wakeAll(&condv);
	env->Mtx->unlock(&lock);
	env->Mem->free(ins);
	return tchOK;
}


/////             Pulse Capture Function                /////
__USER_API__ static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout)
{
	tch_tcapt_handle_proto* ins = (tch_tcapt_handle_proto*) self;
	if(!ins)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_tCaptIsValid(ins))
	{
		return tchErrorParameter;
	}
	const tch_core_api_t* env = ins->env;
	if(tch_port_isISR())
	{
		return tchErrorISR;
	}
	tchEvent evt;
	evt.status = tchOK;
	tch_timer_descriptor* timDesc = &TIMER_HWs[ins->timer];
	uint8_t chMsk = 0;
	if(ch > 1)
	{
		chMsk = 3 << 2;
	}
	else
	{
		chMsk = 3;
	}

	if(!chMsk)
	{
		return tchErrorParameter;
	}
	if((evt.status = env->Mtx->lock(ins->mtx,timeout)) != tchOK)
	{
		return evt.status;
	}
	while(chMsk & timDesc->ch_occp)
	{
		if((evt.status = env->Condv->wait(ins->condv,ins->mtx,timeout)) != tchOK)
		{
			return evt.status;
		}
	}
	timDesc->ch_occp |= chMsk;
	env->Mtx->unlock(ins->mtx);

	tch_msgqId msgq = NULL;
	if(chMsk == 3)
	{
		msgq = ins->msgqs[0];
	}
	else if(chMsk == (3 << 2))
	{
		msgq = ins->msgqs[1];
	}

	while(size--)
	{
		evt = env->MsgQ->get(msgq,timeout);
		if(evt.status != tchEventMessage)
		{
			return evt.status;
		}
		*buf++ = evt.value.v;
	}

	if((evt.status = env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
	{
		return evt.status;
	}
	timDesc->ch_occp &= ~chMsk;
	env->Condv->wakeAll(ins->condv);
	env->Mtx->unlock(ins->mtx);
	return evt.status;
}

__USER_API__ static tchStatus tch_tcapt_setInputEnable(tch_tcaptHandle* self,uint8_t ch,BOOL enable,uint32_t timeout)
{
	if(!self)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_PWMIsValid(self))
	{
		return tchErrorParameter;
	}
	ch = ch / 2;
	tch_tcapt_handle_proto* ins = (tch_tcapt_handle_proto*) self;
	const tch_timer_bs_t* tbs = &TIMER_BD_CFGs[ins->timer];
	tch_hal_module_gpio_t* gpio = (tch_hal_module_gpio_t*) Module->request(MODULE_TYPE_GPIO);
	if(!gpio)
	{
		return tchErrorResource;
	}
	gpio_config_t iocfg;
	if(enable && !ins->iohandle[ch] && (tbs->chp[ch] != -1))
	{
		gpio->initCfg(&iocfg);
		iocfg.Mode = GPIO_Mode_AF;
		iocfg.Af = tbs->afv;
		iocfg.popt = tch_timer_tCaptIsLpActive(ins)? ActOnSleep : NoActOnSleep;
		ins->iohandle[ch] = gpio->allocIo(ins->env,tbs->port,tbs->chp[ch],&iocfg,timeout);
		return tchOK;
	}


	if(!enable && ins->iohandle[ch])
	{
		ins->iohandle[ch]->close(ins->iohandle[ch]);
		ins->iohandle[ch] = NULL;
		return tchOK;
	}
	return tchErrorParameter;
}


__USER_API__ static tchStatus tch_tcapt_close(tch_tcaptHandle* self)
{
	tch_tcapt_handle_proto* ins = (tch_tcapt_handle_proto*) self;
	tchStatus result = tchOK;
	if(!ins)
	{
		return tchErrorParameter;
	}
	if(!tch_timer_tCaptIsValid(ins))
	{
		return tchErrorParameter;
	}
	const tch_core_api_t* env = ins->env;
	if(tch_port_isISR())
	{
		return tchErrorISR;
	}
	tch_timer_descriptor* timDesc = &TIMER_HWs[ins->timer];
	uint8_t chcnt = timDesc->channelCnt / 2;

	if((result = env->Mtx->lock(ins->mtx,tchWaitForever)) != tchOK)
	{
		return result;
	}
	*timDesc->rstr |= timDesc->rstmsk;
	tch_timer_tCaptInvalidate(ins);
	env->Mtx->destroy(ins->mtx);
	env->Condv->destroy(ins->condv);
	uint8_t idx = 0;

	env->MsgQ->destroy(ins->msgqs[0]);
	env->MsgQ->destroy(ins->msgqs[1]);
	env->Mem->free(ins->msgqs);
	while(chcnt--)
	{
		if(ins->iohandle[chcnt])
		{
			ins->iohandle[chcnt]->close(ins->iohandle[chcnt]);
		}
	}
	env->Mem->free(ins->iohandle);

	if((result = env->Mtx->lock(&lock,tchWaitForever)) != tchOK)
	{
		return result;
	}
	timDesc->_handle = NULL;
	*timDesc->rstr &= ~timDesc->rstmsk;
	*timDesc->_clkenr &= ~timDesc->clkmsk;
	*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
	tch_disableInterrupt(timDesc->irq);


	env->Condv->wakeAll(&condv);
	env->Mtx->unlock(&lock);
	env->Mem->free(ins);
	return result;
}

static void tch_timer_handleInterrupt(tch_timer timer)
{
	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;
	uint32_t isr = timerHw->SR;
	if((!timDesc->_handle))
	{
		timerHw->SR &= ~isr;          // clear all raised interrupt
		return;
	}
	if(tch_timer_GPtIsValid(timDesc->_handle))
	{
		if(tch_timer_handle_gptInterrupt(timDesc->_handle,timDesc))
		{
			return;
		}
	}
	if(tch_timer_PWMIsValid(timDesc->_handle))
	{
		if(tch_timer_handle_pwmInterrupt(timDesc->_handle,timDesc))
		{
			return;
		}

	}
	if(tch_timer_tCaptIsValid(timDesc->_handle))
	{
		if(tch_timer_handle_tcaptInterrupt(timDesc->_handle,timDesc))
		{
			return;
		}
	}
	timerHw->SR &= ~isr;          // clear all raised interrupt
}

static BOOL tch_timer_handle_gptInterrupt(tch_gptimer_handle_proto* ins,tch_timer_descriptor* desc)
{
	uint8_t idx = 1;
	TIM_TypeDef* timerHw = (TIM_TypeDef*) desc->_hw;
	const tch_core_api_t* env = ins->env;
	do{
		if(timerHw->SR & (idx << 1))
		{
			timerHw->SR &= ~(idx << 1);        // clear raised interrupt
			timerHw->DIER &= ~(idx << 1);      // clear interrupt enable
			env->MsgQ->put(ins->msgqs[idx - 1],tchOK,0);
			return TRUE;
		}
	}while(idx++ < desc->channelCnt + 1);
	return FALSE;
}

static BOOL tch_timer_handle_pwmInterrupt(tch_pwm_handle_proto* ins,tch_timer_descriptor* desc)
{
	TIM_TypeDef* timerHw = (TIM_TypeDef*)desc->_hw;
	timerHw->SR &= ~TIM_SR_UIF;
	//return ins->env->Rendezvous->wake(ins->uev_rndv) == tchOK;
	// TODO: replace barrier
	return ins->env->WaitQ->wake(ins->uev_wq) == tchOK;
}


static BOOL tch_timer_handle_tcaptInterrupt(tch_tcapt_handle_proto* ins, tch_timer_descriptor* desc)
{
	TIM_TypeDef* timerHw = (TIM_TypeDef*) desc->_hw;
	const tch_core_api_t* env = ins->env;
	uint32_t v = 0;
	uint32_t sr = timerHw->SR;
	if(sr & TIM_SR_CC1IF)
	{
		v = timerHw->CCR1;
		env->MsgQ->put(ins->msgqs[0],timerHw->CCR2,0);
		return TRUE;
	}
	else if(sr & TIM_SR_CC3IF)
	{
		v = timerHw->CCR3;
		env->MsgQ->put(ins->msgqs[1],timerHw->CCR4,0);
		return TRUE;
	}
	return FALSE;
}


void TIM2_IRQHandler(void)
{                   /* TIM2                         */
	tch_timer_handleInterrupt(0);
}

void TIM3_IRQHandler(void)
{
	tch_timer_handleInterrupt(1);
}

void TIM4_IRQHandler(void)
{
	tch_timer_handleInterrupt(2);
}

void TIM5_IRQHandler(void)
{
	tch_timer_handleInterrupt(3);
}

void TIM1_BRK_TIM9_IRQHandler(void)
{
	tch_timer_handleInterrupt(4);
}

void TIM1_UP_TIM10_IRQHandler(void)
{
	tch_timer_handleInterrupt(5);
}

void TIM1_TRG_COM_TIM11_IRQHandler(void)
{
	tch_timer_handleInterrupt(6);
}

void TIM8_BRK_TIM12_IRQHandler(void)
{         /* TIM8 Break and TIM12         */
	tch_timer_handleInterrupt(7);
}

void TIM8_UP_TIM13_IRQHandler(void)
{          /* TIM8 Update and TIM13        */
	tch_timer_handleInterrupt(8);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void)
{     /* TIM8 Trigger and Commutation and TIM14 */
	tch_timer_handleInterrupt(9);
}

