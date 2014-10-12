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
#include "tch_kernel.h"
#include "tch_timer.h"
#include "tch_halInit.h"



#define TIMER_UNITTIME_mSEC      ((uint8_t) 0)
#define TIMER_UNITTIME_uSEC      ((uint8_t) 1)
#define TIMER_UNITTIME_nSEC      ((uint8_t) 2)

#define INIT_UNTITIME_STR        {\
	                              TIMER_UNITTIME_mSEC,\
	                              TIMER_UNITTIME_uSEC,\
	                              TIMER_UNITTIME_nSEC\
}


/*     class identifier for checking validity of instance    */
#define TIMER_GP_CLASS_KEY       ((uint16_t) 0x6401)
#define TIMER_PWM_CLASS_KEY      ((uint16_t) 0x6411)
#define TIMER_CAPTURE_CLASS_KEY  ((uint16_t) 0x6421)

#define tch_timer_GPtValidate(gpt_ins)           do{\
	((tch_gptimer_handle_proto*) gpt_ins)->key = ((gpt_ins & 0xFFFF) ^ TIMER_GP_CLASS_KEY);\
}while(0)

#define tch_timer_GPtInvalidate(gpt_ins)        do{\
	((tch_gptimer_handle_proto*) gpt_ins)->key = 0;\
}while(0)

#define tch_timer_GPtIsValid(gpt_ins)    (((tch_gptimer_handle_proto*) gpt_ins)->key == ((gpt_ins & 0xFFFF) ^ TIMER_GP_CLASS_KEY))


#define tch_timer_PWMValidate(pwm_ins)           do{\
	((tch_pwm_handle_proto*) pwm_ins)->key = ((pwm_ins & 0xFFFF) ^ TIMER_PWM_CLASS_KEY);\
}while(0)

#define tch_timer_PWMInvalidate(pwm_ins)        do{\
	((tch_pwm_handle_proto*) pwm_ins)->key = 0;\
}while(0)

#define tch_timer_PWMIsValid(pwm_ins)    (((tch_pwm_handle_proto*) pwm_ins)->key == ((pwm_ins & 0xFFFF) ^ TIMER_PWM_CLASS_KEY))


#define tch_timer_tCaptValidate(capt_ins)           do{\
	((tch_tcapt_handle_proto*) capt_ins)->key = ((capt_ins & 0xFFFF) ^ TIMER_CAPTURE_CLASS_KEY);\
}while(0)

#define tch_timer_tCaptInvalidate(capt_ins)        do{\
	((tch_tcapt_handle_proto*) capt_ins)->key = 0;\
}while(0)

#define tch_timer_tCaptIsValid(capt_ins)    (((tch_tcapt_handle_proto*) capt_ins)->key == ((capt_ins & 0xFFFF) ^ TIMER_CAPTURE_CLASS_KEY))



typedef struct tch_timer_mgr_t {
	tch_lld_timer                     pix;
	tch_mtxId                         mtx;
	tch_condvId                       condv;
} tch_timer_manager;


struct tch_gptimer_req_t {
	uint32_t utick;
	void* ins;
};

typedef struct tch_gptimer_handle_proto_t {
	tch_gptimerHandle     _pix;
	uint16_t               key;
	tch_asyncId            async;
	tch_timer              timer;
	const tch*             env;
}tch_gptimer_handle_proto;

typedef struct tch_pwm_handle_proto_t{
	tch_pwmHandle         _pix;
	uint16_t               key;
}tch_pwm_handle_proto;

typedef struct tch_tcapt_handle_proto_t{
	tch_tcaptHandle       _pix;
	uint16_t               key;
}tch_tcapt_handle_proto;


///////            Timer Manager Function               ///////
static tch_gptimerHandle* tch_timer_allocGptimerUnit(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def,uint32_t timeout);
static tch_pwmHandle* tch_timer_allocPWMUnit(const tch* env,tch_timer timer,tch_pwmDef* tdef,uint32_t timeout);
static tch_tcaptHandle* tch_timer_allocCaptureUnit(const tch* env,tch_timer timer,tch_tcaptDef* tdef,uint32_t timeout);
static uint32_t tch_timer_getChannelCount(tch_timer timer);
static uint8_t tch_timer_getPrecision(tch_timer timer);
static void tch_timer_handleInterrupt(tch_timer timer);




//////         General Purpose Timer Function           ///////
static DECL_ASYNC_TASK(tch_gptimer_trigger);
static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t utick);
static tchStatus tch_gptimer_close(tch_gptimerHandle* self);
static BOOL tch_timer_handle_gptInterrupt(tch_gptimer_handle_proto* ins,tch_timer_descriptor* desc);


//////            PWM fucntion                        //////
static BOOL tch_pwm_start(tch_pwmHandle* self,uint32_t ch);
static BOOL tch_pwm_stop(tch_pwmHandle* self,uint32_t ch);
static uint32_t tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch);
static uint32_t tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch);
static tchStatus tch_pwm_close(tch_pwmHandle* self);


/////             Pulse Capture Function                /////
static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout);
static tchStatus tch_tcapt_close(tch_tcaptHandle* self);




__attribute__((section(".data")))  static tch_timer_manager TIMER_StaticInstance = {
		{
				{0,1,2,3,4,5,6,7,8,9},
				INIT_UNTITIME_STR,
				tch_timer_allocGptimerUnit,
				tch_timer_allocPWMUnit,
				tch_timer_allocCaptureUnit,
				tch_timer_getChannelCount,
				tch_timer_getPrecision
		},
		NULL,
		NULL
};


const tch_lld_timer* tch_timer_instance = (tch_lld_timer*) &TIMER_StaticInstance;

///////            Timer Manager Function               ///////
static tch_gptimerHandle* tch_timer_allocGptimerUnit(const tch* env,tch_timer timer,tch_gptimerDef* gpt_def,uint32_t timeout){
	uint16_t tmpccer = 0, tmpccmr = 0;
	if(!TIMER_StaticInstance.condv)
		TIMER_StaticInstance.condv = env->Condv->create();
	if(!TIMER_StaticInstance.mtx)
		TIMER_StaticInstance.mtx = env->Mtx->create();

	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];
	tch_gptimer_handle_proto* ins = env->Mem->alloc(sizeof(tch_gptimer_handle_proto));
	if(env->Mtx->lock(TIMER_StaticInstance.mtx,timeout) != osOK){
		env->Mem->free(ins);
		return NULL;
	}
	while(timDesc->_handle){
		if(env->Condv->wait(TIMER_StaticInstance.condv,TIMER_StaticInstance.mtx,timeout) != osOK)
			return NULL;
	}
	timDesc->_handle = (void*)1; /// mark as occupied
	env->Mtx->unlock(TIMER_StaticInstance.mtx);

	/* bind instance method and internal member var  */
	ins->_pix.close = tch_gptimer_close;
	ins->_pix.wait = tch_gptimer_wait;
	ins->async = env->Async->create(timDesc->channelCnt);
	ins->env = env;
	ins->timer = timer;
	tch_timer_GPtValidate(ins);

	timDesc->_handle = ins;
	TIM_TypeDef* timerHw = (TIM_TypeDef*)timDesc->_hw;

	//enable clock
	*timDesc->_clkenr |= timDesc->clkmsk;
	if(gpt_def->pwrOpt == ActOnSleep)
		*timDesc->_lpclkenr |= timDesc->lpclkmsk;

	uint32_t psc = 2;
	if(timDesc->_clkenr == &RCC->APB1ENR)
		psc = 4;

	switch(gpt_def->UnitTime){
	case TIMER_UNITTIME_mSEC:
		timerHw->PSC = SYS_CLK / psc / 1000;
		break;
	case TIMER_UNITTIME_nSEC:   // out of capability
	case TIMER_UNITTIME_uSEC:
		timerHw->PSC = SYS_CLK / psc / 1000000;
	}

	if(timDesc->channelCnt > 0){   // if requested timer h/w supprot channel 1 initialize its related registers

		timerHw->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);  // clear ccer output enable
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;

		tmpccer &= ~(TIM_CCER_CC1NP | TIM_CCER_CC1P);
		tmpccmr &= ~(TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);   //* set capture/compare as frozen mode and configured as output
		if((timerHw == TIM2) || (timerHw == TIM8)){
			timerHw->CR2 &= ~(TIM_CR2_OIS1 | TIM_CR2_OIS1N);
		}
		timerHw->CCER = tmpccer;
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCR1 = 0;
	}

	if(timDesc->channelCnt > 1){   // same as above for channel 2
		timerHw->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR1;

		tmpccer &= ~(TIM_CCER_CC2NP | TIM_CCER_CC2P);
		tmpccmr &= ~(TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
		if((timerHw == TIM2) || (timerHw == TIM8)){
			timerHw->CR2 &= ~(TIM_CR2_OIS2 | TIM_CR2_OIS2N);
		}
		timerHw->CCER = tmpccer;
		timerHw->CCMR1 = tmpccmr;
		timerHw->CCR2 = 0;
	}
	if(timDesc->channelCnt > 2){   //.. for channel 3
		timerHw->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3NE);
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC3NP | TIM_CCER_CC3P);
		tmpccmr &= ~(TIM_CCMR2_CC3S | TIM_CCMR2_OC3M);
		if((timerHw == TIM2) || (timerHw == TIM8)){
			timerHw->CR2 &= ~(TIM_CR2_OIS3 | TIM_CR2_OIS3N);
		}
		timerHw->CCER = tmpccer;
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCR3 = 0;
	}
	if(timDesc->channelCnt > 3){   // .. for channel 4
		timerHw->CCER &= ~TIM_CCER_CC4E;
		tmpccer = timerHw->CCER;
		tmpccmr = timerHw->CCMR2;

		tmpccer &= ~(TIM_CCER_CC4NP | TIM_CCER_CC4P);
		tmpccmr &= ~(TIM_CCMR2_CC4S | TIM_CCMR2_OC4M);
		if((timerHw == TIM2) || (timerHw == TIM8)){
			timerHw->CR2 &= ~(TIM_CR2_OIS4);
		}
		timerHw->CCER = tmpccer;
		timerHw->CCMR2 = tmpccmr;
		timerHw->CCR4 = 0;
	}
	timerHw->CNT = 0;
	timerHw->ARR = 0;
	timerHw->CR1 = TIM_CR1_CEN;

	return (tch_gptimerHandle*) ins;
}

static tch_pwmHandle* tch_timer_allocPWMUnit(const tch* env,tch_timer timer,tch_pwmDef* tdef,uint32_t timeout){

}

static tch_tcaptHandle* tch_timer_allocCaptureUnit(const tch* env,tch_timer timer,tch_tcaptDef* tdef,uint32_t timeout){

}

static uint32_t tch_timer_getChannelCount(tch_timer timer){

}

static uint8_t tch_timer_getPrecision(tch_timer timer){

}


//////         General Purpose Timer Function           ///////

static DECL_ASYNC_TASK(tch_gptimer_trigger){
	struct tch_gptimer_req_t* gpt_arg = (struct tch_gptimer_req_t*) arg;
	tch_gptimer_handle_proto* ins = (tch_gptimer_handle_proto*) gpt_arg->ins;
	TIM_TypeDef* thw = (TIM_TypeDef*)TIMER_HWs[ins->timer]._hw;
	uint32_t vcnt = thw->CNT + gpt_arg->utick;
	switch(id){
	case 0:
		thw->DIER |= TIM_DIER_CC1IE;
		return TRUE;
	case 1:
		thw->DIER |= TIM_DIER_CC2IE;
		return TRUE;
	case 2:
		thw->DIER |= TIM_DIER_CC3IE;
		return TRUE;
	case 3:
		thw->DIER |= TIM_DIER_CC4IE;
		return TRUE;
	}
	return FALSE;
}

// Not Thread-Safe
static BOOL tch_gptimer_wait(tch_gptimerHandle* self,uint32_t utick){
	tch_gptimer_handle_proto* ins = (tch_gptimer_handle_proto*) self;
	if(!tch_timer_GPtIsValid(ins))
		return FALSE;
	tch_timer_descriptor* thw = &TIMER_HWs[ins->timer];
	TIM_TypeDef* timerHw = thw->_hw;
	uint8_t idx = 0;
	int id = 0;
	uint8_t chmsk = thw->ch_occp;
	const tch* env = ins->env;
	struct tch_gptimer_req_t req;
	req.ins = self;
	req.utick = utick;
	do{
		if(!(chmsk & 1)){
			thw->ch_occp |= (1 << idx); // set occp bit
			id = idx;
			switch(idx){
			case 0:
				timerHw->CCR1 = timerHw->CNT + utick;
				break;
			case 1:
				timerHw->CCR2 = timerHw->CNT + utick;
				break;
			case 2:
				timerHw->CCR3 = timerHw->CNT + utick;
				break;
			case 3:
				timerHw->CCR4 = timerHw->CNT + utick;
				break;
			}
			return env->Async->wait(ins->async,id,tch_gptimer_trigger,osWaitForever,&req) == osOK? TRUE : FALSE;
		}
		chmsk >>= 1;
	}while(idx++ < thw->channelCnt);
	return FALSE;   // couldn't find available timer channel ( all occupied)
}

static tchStatus tch_gptimer_close(tch_gptimerHandle* self){
	tch_gptimer_handle_proto* ins = (tch_gptimer_handle_proto*) self;
	tch_timer_descriptor* timDesc = &TIMER_HWs[ins->timer];
	const tch* env = ins->env;
	tchStatus result = osOK;
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;
	timerHw->CR1 &= ~TIM_CR1_CEN;                // disable timer count
	if((result = env->Mtx->lock(TIMER_StaticInstance.mtx,osWaitForever)) != osOK)
		return result;
	*timDesc->_clkenr &= ~timDesc->clkmsk;
	*timDesc->_lpclkenr &= ~timDesc->lpclkmsk;
	timDesc->_handle = NULL;
	env->Condv->wakeAll(TIMER_StaticInstance.condv);
	env->Mtx->unlock(TIMER_StaticInstance.mtx);

	env->Async->destroy(ins->async);
	env->Mem->free(ins);
	return osOK;
}


//////            PWM fucntion                        //////
static BOOL tch_pwm_start(tch_pwmHandle* self,uint32_t ch){

}

static BOOL tch_pwm_stop(tch_pwmHandle* self,uint32_t ch){

}

static uint32_t tch_pwm_setDuty(tch_pwmHandle* self,uint32_t ch){

}

static uint32_t tch_pwm_getDuty(tch_pwmHandle* self,uint32_t ch){

}

static tchStatus tch_pwm_close(tch_pwmHandle* self){

}


/////             Pulse Capture Function                /////
static tchStatus tch_tcapt_read(tch_tcaptHandle* self,uint8_t ch,uint32_t* buf,size_t size,uint32_t timeout){

}

static tchStatus tch_tcapt_close(tch_tcaptHandle* self){

}

static void tch_timer_handleInterrupt(tch_timer timer){
	tch_timer_descriptor* timDesc = &TIMER_HWs[timer];
	TIM_TypeDef* timerHw = (TIM_TypeDef*) timDesc->_hw;
	uint32_t isr = timerHw->SR;
	if((!timDesc->_handle)){
		timerHw->SR &= ~isr;          // clear all raised interrupt
		return;
	}
	if(tch_timer_GPtIsValid(timDesc->_handle)){
		if(tch_timer_handle_gptInterrupt(timDesc->_handle,timDesc))
			return;
	}
	timerHw->SR &= ~isr;          // clear all raised interrupt
}

static BOOL tch_timer_handle_gptInterrupt(tch_gptimer_handle_proto* ins,tch_timer_descriptor* desc){
	uint8_t idx = 0;
	TIM_TypeDef* timerHw = (TIM_TypeDef*) desc->_hw;
	tch* env = ins->env;
	while(idx++ < desc->channelCnt){
		if(timerHw->SR & (idx << 1)){
			timerHw->SR &= ~(idx << 1);        // clear raised interrupt
			timerHw->DIER &= ~(idx << 1);      // clear interrupt enable
			env->Async->notify(ins->async,idx,osOK);  // notify to waiting thread
			return TRUE;
		}
	}
	return FALSE;
}

void TIM2_IRQHandler(void){                   /* TIM2                         */
	tch_timer_handleInterrupt(0);
}

void TIM3_IRQHandler(void){
	tch_timer_handleInterrupt(1);
}

void TIM4_IRQHandler(void){
	tch_timer_handleInterrupt(2);
}

void TIM5_IRQHandler(void){
	tch_timer_handleInterrupt(3);
}

void TIM1_BRK_TIM9_IRQHandler(void){
	tch_timer_handleInterrupt(4);
}

void TIM1_UP_TIM10_IRQHandler(void){
	tch_timer_handleInterrupt(5);
}

void TIM1_TRG_COM_TIM11_IRQHandler(void){
	tch_timer_handleInterrupt(6);
}

void TIM8_BRK_TIM12_IRQHandler(void){         /* TIM8 Break and TIM12         */
	tch_timer_handleInterrupt(7);
}

void TIM8_UP_TIM13_IRQHandler(void){          /* TIM8 Update and TIM13        */
	tch_timer_handleInterrupt(8);
}

void TIM8_TRG_COM_TIM14_IRQHandler(void){     /* TIM8 Trigger and Commutation and TIM14 */
	tch_timer_handleInterrupt(9);
}

