/*
 * tch_gpio.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 27.
 *      Author: innocentevil
 */


#include <stdlib.h>
#include "tch_gpio.h"
#include "tch_hal.h"

#include "kernel/tch_kmod.h"
#include "kernel/tch_mtx.h"
#include "kernel/tch_condv.h"
#include "kernel/tch_ktypes.h"
#include "kernel/tch_kernel.h"



#define GPIO_Event_Signal              ((int32_t) 1)


#define GPIO_Mode_Msk                  (GPIO_Mode_IN |\
		                                GPIO_Mode_OUT|\
		                                GPIO_Mode_AF|\
		                                GPIO_Mode_AN)

#define GPIO_Otype_Msk                  (GPIO_Otype_PP | GPIO_Otype_OD)


#define GPIO_OSpeed_Msk                 (GPIO_OSpeed_2M|\
		                                 GPIO_OSpeed_25M|\
		                                 GPIO_OSpeed_50M|\
		                                 GPIO_OSpeed_100M)


#define GPIO_PuPd_Msk                   (GPIO_PuPd_Float|\
		                                 GPIO_PuPd_PU|\
		                                 GPIO_PuPd_PD)


#define GPIO_EvEdge_Both               ((uint8_t) GPIO_EvEdge_Fall | GPIO_EvEdge_Rise)





#define GPIO_INTERRUPT_INIT      ((uint32_t) 0xF8 << 16)
#define GPIO_INTERRUPT_INIT_MSK  ((uint32_t) 0xFF << 16)
#define GPIO_CLASS_KEY  	     ((uint16_t) 0x3D03)


#define SET_INTERRUPT(gpio_handle)  ((tch_gpio_handle_prototype*) gpio_handle)->state |= GPIO_INTERRUPT_INIT
#define CLR_INTERRUPT(gpio_handle)  ((tch_gpio_handle_prototype*) gpio_handle)->state &= ~GPIO_INTERRUPT_INIT_MSK
#define IS_INTERRUPT(gpio_handle)   ((((tch_gpio_handle_prototype*) gpio_handle)->state & GPIO_INTERRUPT_INIT_MSK) == GPIO_INTERRUPT_INIT)


typedef struct tch_gpio_manager_internal_t {
	tch_hal_module_gpio_t                 _pix;
	tch_mtxId                     mtxId;
	tch_condvId                   condvId;
	const uint8_t                 port_cnt;
	const uint8_t                 io_cnt;
}tch_gpio_manager;

typedef struct tch_gpio_handle_prototype_t {
	tch_gpioHandle               _pix;
	uint8_t                       idx;
	uint32_t                      state;
	uint32_t                      pMsk;
	tch_mtxId                     mtxId;
	tch_IoEventCallback_t         cb;
	const tch_core_api_t*                    env;
}tch_gpio_handle_prototype;



/////////////////////////////////////    public function     //////////////////////////////////


/**********************************************************************************************/
/************************************  gpio driver public function  ***************************/
/**********************************************************************************************/
__USER_API__ static tch_gpioHandle* tch_gpio_alloc(const tch_core_api_t* env,const gpIo_x port,uint32_t pmsk,const gpio_config_t* cfg,uint32_t timeout);
__USER_API__ static tchStatus tch_gpio_free(tch_gpioHandle* self);
__USER_API__ static void tch_gpio_initConfig(gpio_config_t* cfg);
__USER_API__ static void tch_gpio_initEvConfig(gpio_event_config_t* evcfg);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/
__USER_API__ static void tch_gpio_out(tch_gpioHandle* self,uint32_t pmsk,tch_bState nstate);
__USER_API__ static tch_bState tch_gpio_in(tch_gpioHandle* self,uint32_t pmsk);
__USER_API__ static tchStatus tch_gpio_configure(tch_gpioHandle* self,const gpio_config_t* cfg,uint32_t pmsk);
__USER_API__ static tchStatus tch_gpio_registerEvent(tch_gpioHandle* self,pin p,const gpio_event_config_t* cfg);
__USER_API__ static tchStatus tch_gpio_unregisterEvent(tch_gpioHandle* self,pin p);
__USER_API__ static tchStatus tch_gpio_configureEvent(tch_gpioHandle* self,pin p,const gpio_event_config_t* cfg);
__USER_API__ static BOOL tch_gpio_waitEvent(tch_gpioHandle* self,uint8_t pin,uint32_t timeout);



/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static int tch_gpio_init(void);
static void tch_gpio_exit(void);
static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt);
static inline void tch_gpio_validate(tch_gpio_handle_prototype* _handle);
static inline void tch_gpio_invalidate(tch_gpio_handle_prototype* _handle);
static inline BOOL tch_gpio_isValid(tch_gpio_handle_prototype* _handle);

__USER_RODATA__ tch_hal_module_gpio_t GPIO_Ops = {
		.count = MFEATURE_GPIO,
		.allocIo = tch_gpio_alloc,
		.initCfg = tch_gpio_initConfig,
		.initEvCfg = tch_gpio_initEvConfig
};

static tch_mtxCb lock;
static tch_condvCb condv;


static int tch_gpio_init(void)
{
	tch_mutexInit(&lock);
	tch_condvInit(&condv);
	tch_kmod_register(MODULE_TYPE_GPIO,GPIO_CLASS_KEY,&GPIO_Ops,FALSE);
	uint32_t i;
	return TRUE;
}

static void tch_gpio_exit(void)
{
	tch_kmod_unregister(MODULE_TYPE_GPIO,GPIO_CLASS_KEY);
	uint32_t i;
	tch_mutexDeinit(&lock);
	tch_condvDeinit(&condv);
}

MODULE_INIT(tch_gpio_init);
MODULE_EXIT(tch_gpio_exit);


static tch_gpioHandle* tch_gpio_alloc(const tch_core_api_t* env,const gpIo_x port,uint32_t pmsk,const gpio_config_t* cfg,uint32_t timeout)
{
	tch_gpio_descriptor* gpio = &GPIO_HWs[port];
	if(!gpio->_clkenr)                   /// given GPIO port is not supported in this H/W
		return NULL;

	if(env->Mtx->lock(&lock,timeout) != tchOK)
		return NULL;
	while(gpio->io_ocpstate & pmsk)
	{           /// if there is pin which is occupied by another instance
		if(env->Condv->wait(&condv,&lock,timeout) != tchOK)
			return NULL;
	}
	gpio->io_ocpstate |= pmsk;     // mark pin as occupied
	env->Mtx->unlock(&lock);

	tch_gpio_handle_prototype* ins = env->Mem->alloc(sizeof(tch_gpio_handle_prototype));   // allocate gpio handle
	ins->env = env;
	ins->_pix.close = tch_gpio_free;
	ins->_pix.in = tch_gpio_in;
	ins->_pix.out = tch_gpio_out;
	ins->_pix.listen = tch_gpio_waitEvent;
	ins->_pix.registerIoEvent = tch_gpio_registerEvent;
	ins->_pix.unregisterIoEvent = tch_gpio_unregisterEvent;
	ins->_pix.configure = tch_gpio_configure;
	ins->_pix.configureEvent = tch_gpio_configureEvent;
	ins->pMsk = pmsk;
	ins->mtxId = env->Mtx->create();
	ins->cb = NULL;
	ins->idx = port;

	*gpio->_clkenr |= gpio->clkmsk;              /// enable gpio clock
	if(cfg->popt == ActOnSleep)
	{                      /// if active-on-sleep is enabled
		*gpio->_lpclkenr |= gpio->lpclkmsk;      /// set lp clk bit
	}
	else
	{
		*gpio->_lpclkenr &= ~gpio->lpclkmsk;     /// otherwise clear
	}

	tch_gpio_configure((tch_gpioHandle*) ins,cfg,pmsk);
	tch_gpio_validate(ins);
	return (tch_gpioHandle*) ins;
}

static void tch_gpio_initConfig(gpio_config_t* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_Mode_IN;
	cfg->Speed = GPIO_OSpeed_2M;
	cfg->Otype = GPIO_Otype_OD;
	cfg->PuPd = GPIO_PuPd_Float;
	cfg->popt = ActOnSleep;
}

static void tch_gpio_initEvConfig(gpio_event_config_t* evcfg){
	evcfg->EvEdge = GPIO_EvEdge_Fall;
	evcfg->EvType = GPIO_EvType_Interrupt;
	evcfg->EvCallback = NULL;
}


static tchStatus tch_gpio_free(tch_gpioHandle* self){
	tch_gpio_handle_prototype* ins = NULL;
	tchStatus result = 0;
	if(!self)
		return tchErrorParameter;
	ins = (tch_gpio_handle_prototype*) self;
	if(!tch_gpio_isValid(ins))
		return tchErrorResource;
	const tch_core_api_t* env = ins->env;
	if(env->Mtx->lock(ins->mtxId,tchWaitForever) != tchOK)
		return tchErrorTimeoutResource;

	tch_gpio_descriptor* gpio = &GPIO_HWs[ins->idx];
	gpio_config_t initCfg;
	tch_gpio_initConfig(&initCfg);
	tch_gpio_configure(self,&initCfg,ins->pMsk);
	uint16_t pmsk = ins->pMsk;
	pin p = 0;
	while(pmsk)
	{
		tch_gpio_unregisterEvent(self,p);
		pmsk >>= 1;
		p++;
	}

	tch_gpio_invalidate(ins);
	env->Mtx->destroy(ins->mtxId);

	env->Mtx->lock(&lock,tchWaitForever);
	*gpio->_clkenr &= ~gpio->clkmsk;
	*gpio->_lpclkenr &= ~gpio->lpclkmsk;
	tch_gpio_invalidate(ins);
	gpio->io_ocpstate &= ~ins->pMsk;
	env->Condv->wakeAll(&condv);
	env->Mtx->unlock(&lock);
	env->Mem->free(ins);
	return tchOK;
}

static void tch_gpio_out(tch_gpioHandle* self,uint32_t pmsk,tch_bState nstate){
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if(!ins)
		return;
	if(!tch_gpio_isValid(ins))
		return;
	if(!(ins->pMsk & pmsk))
		return;
	if(nstate)
	{
		((GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw)->ODR |= pmsk;
	}
	else
	{
		((GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw)->ODR &= ~pmsk;
	}
}

static tch_bState tch_gpio_in(tch_gpioHandle* self,uint32_t pmsk)
{
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return bClear;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpio_isValid(_handle))
		return bClear;
	if(!(_handle->pMsk & pmsk))
		return bClear;
	return ((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->IDR & pmsk ? bSet : bClear;
}

static tchStatus tch_gpio_registerEvent(tch_gpioHandle* self,pin p,const gpio_event_config_t* cfg)
{
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[p];

	if(!self)
		return tchErrorParameter;
	if(!tch_gpio_isValid(ins))
		return tchErrorResource;

	if(__get_IPSR())
		return tchErrorISR;


	const tch_core_api_t* env = ins->env;
	if(env->Mtx->lock(&lock,tchWaitForever) != tchOK)
	{
		return tchErrorResource;
	}

	if(ioIrqObj->io_occp)
	{
		env->Mtx->unlock(&lock);
		return tchErrorResource;
	}
	tch_rendzvInit(&ioIrqObj->rendezv);

	ioIrqObj->io_occp = ins;
	SET_INTERRUPT(ins);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	ins->cb = cfg->EvCallback;
	tch_gpio_configureEvent(self,p,cfg);
	env->Mtx->unlock(&lock);
	return tchOK;
}

static tchStatus tch_gpio_unregisterEvent(tch_gpioHandle* self,pin p)
{
	tch_gpio_handle_prototype* ins = NULL;
	if(!self)
		return tchErrorParameter;
	ins = (tch_gpio_handle_prototype* )self;
	if(!tch_gpio_isValid(ins))
		return tchErrorResource;
	if(!IS_INTERRUPT(ins))
		return tchErrorParameter;
	if(__get_IPSR())
		return tchErrorISR;

	tch_ioInterrupt_descriptor* ioIntrDesc = &IoInterrupt_HWs[p];
	if(ioIntrDesc->io_occp != self)
		return tchErrorParameter;

	const tch_core_api_t* api = ins->env;
	if(api->Mtx->lock(&lock,tchWaitForever) != tchOK)   // lock mtx of singleton
		return tchErrorResource;

	uint16_t pmsk = (1 << p);
	ioIntrDesc->io_occp = NULL;
	tch_rendzvDeinit(&ioIntrDesc->rendezv);

	// TODO: replace barrier
	tch_disableInterrupt(ioIntrDesc->irq);
	//NVIC_DisableIRQ(ioIntrDesc->irq);
	SYSCFG->EXTICR[p >> 2] = 0;
	EXTI->EMR &= ~pmsk;
	EXTI->IMR &= ~pmsk;
	EXTI->FTSR &= ~pmsk;
	EXTI->RTSR &= ~pmsk;
	ins->cb = NULL;
	CLR_INTERRUPT(ins);    // clear interrupt flag in status
	api->Mtx->unlock(&lock);   // release mtx of singleton
	return tchOK;

}


static tchStatus tch_gpio_configure(tch_gpioHandle* self,const gpio_config_t* cfg,uint32_t pmsk)
{
	tch_gpio_handle_prototype* ins = NULL;
	GPIO_TypeDef* ioHw = NULL;
	uint8_t pin = 0;
	uint32_t p_msk = 1;

	if(!self)
		return tchErrorParameter;
	 ins = (tch_gpio_handle_prototype*) self;
	if(!tch_gpio_isValid(ins))
		return tchErrorResource;
	if(!(ins->pMsk & pmsk))
		return tchErrorParameter;
	ioHw = (GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw;
	while(pmsk)
	{
		if(pmsk & 0x01)
		{
			ioHw->MODER &= ~(GPIO_Mode_Msk << (pin << 1));
			ioHw->OSPEEDR &= ~(GPIO_OSpeed_Msk << (pin << 1));
			ioHw->OSPEEDR |= (cfg->Speed << (pin << 1));
			switch(cfg->Mode){
			case GPIO_Mode_OUT:             /// gpio configure to output
				ioHw->MODER |= (GPIO_Mode_OUT << (pin << 1));

				ioHw->OTYPER &= ~(GPIO_Otype_Msk << pin);
				if(cfg->Otype == GPIO_Otype_OD)
					ioHw->OTYPER |= (1 << pin);
				break;
			case GPIO_Mode_IN:
				ioHw->MODER |= (GPIO_Mode_IN << (pin << 1));
				ioHw->PUPDR &= ~(GPIO_PuPd_Msk << (pin << 1));
				ioHw->PUPDR |= (cfg->PuPd << (pin << 1));
				break;
			case GPIO_Mode_AN:
				ioHw->MODER |= (GPIO_Mode_AN << (pin  << 1));
				break;
			case GPIO_Mode_AF:
				ioHw->MODER |= (GPIO_Mode_AF << (pin << 1));
				if(pin < 8){
					ioHw->AFR[0] &= ~(0xF << (pin << 2));
					ioHw->AFR[0] |= (cfg->Af << (pin << 2));
				}else{
					ioHw->AFR[1] &= ~(0xF << ((pin - 8) << 2));
					ioHw->AFR[1] |= (cfg->Af << ((pin - 8) << 2));
				}
			}
		}
		pin++;
		pmsk >>= 1;
	}
	return tchOK;
}

static BOOL tch_gpio_waitEvent(tch_gpioHandle* self,uint8_t pin,uint32_t timeout)
{
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if((!self) || (!tch_gpio_isValid(ins)) || (__get_IPSR()))
		return FALSE;
	uint32_t pmsk = (1 << pin);
	if(!(ins->pMsk & pmsk))
		return FALSE;
	const tch_core_api_t* env = ins->env;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[pin];
	if(ioIrqObj->io_occp != self)
		return FALSE;
	return env->Rendezvous->sleep(&ioIrqObj->rendezv,timeout) == tchOK;
}



static tchStatus tch_gpio_configureEvent(tch_gpioHandle* self,uint8_t pin,const gpio_event_config_t* cfg)
{
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if((!self) || (!tch_gpio_isValid(ins)) || (__get_IPSR()))
		return tchErrorParameter;
	uint16_t pmsk = (1 << pin);
	if(!(ins->pMsk & pmsk))
		return tchErrorParameter;
	tch_ioInterrupt_descriptor* ioDesc = &IoInterrupt_HWs[pin];

	tch_disableInterrupt(ioDesc->irq);
	//NVIC_DisableIRQ(ioDesc->irq);
	EXTI->RTSR &= ~pmsk;
	EXTI->FTSR &= ~pmsk;

	if(cfg->EvEdge & GPIO_EvEdge_Rise)
	{
		EXTI->RTSR |= pmsk;
	}
	if(cfg->EvEdge & GPIO_EvEdge_Fall)
	{
		EXTI->FTSR |= pmsk;
	}

	EXTI->EMR &= ~pmsk;
	EXTI->IMR &= ~pmsk;
	if(cfg->EvType & GPIO_EvType_Event)
	{
		EXTI->EMR |= pmsk;
	}
	if(cfg->EvType & GPIO_EvType_Interrupt)
	{
		EXTI->IMR |= pmsk;
	}
	tch_enableInterrupt(ioDesc->irq,HANDLER_NORMAL_PRIOR);
	SYSCFG->EXTICR[pin >> 2] |= ins->idx << ((pin % 4) *4);
	return tchOK;
}


static inline void tch_gpio_validate(tch_gpio_handle_prototype* _handle)
{
	_handle->state = ((uint32_t) _handle & 0xFFFF) ^ GPIO_CLASS_KEY;
}

static inline void tch_gpio_invalidate(tch_gpio_handle_prototype* _handle)
{
	_handle->state &= ~(0xFFFF);
}

static inline BOOL tch_gpio_isValid(tch_gpio_handle_prototype* _handle)
{
	return (_handle->state & 0xFFFF) == ((uint32_t) _handle & 0xFFFF) ^ GPIO_CLASS_KEY;
}

static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt)
{
	uint32_t ext_pr = (EXTI->PR >> base_idx);
	tch_ioInterrupt_descriptor* ioIntObj = NULL;
	tch_gpio_handle_prototype* _handle = NULL;
	uint8_t pos = 0;
	uint32_t pMsk = 1;
	while(ext_pr)
	{
		if(ext_pr & 1)
		{
			ioIntObj = &IoInterrupt_HWs[base_idx + pos];
			if(!ioIntObj->io_occp)
				return;
			_handle = (tch_gpio_handle_prototype*)ioIntObj->io_occp;
			if(_handle->cb)
				_handle->cb((tch_gpioHandle*)_handle,base_idx + pos);
			EXTI->PR |= pMsk;
			_handle->env->Rendezvous->wake(&ioIntObj->rendezv);
		}
		ext_pr >>= 1;
		pMsk <<= 1;
		if(!(++pos < group_cnt))
			return;
	}
}


void EXTI0_IRQHandler(void)
{
	tch_gpio_handleIrq(0,1);
}

void EXTI1_IRQHandler(void)
{
	tch_gpio_handleIrq(1,1);
}

void EXTI2_IRQHandler(void)
{
	tch_gpio_handleIrq(2,1);
}

void EXTI3_IRQHandler(void)
{
	tch_gpio_handleIrq(3,1);
}

void EXTI4_IRQHandler(void)
{
	tch_gpio_handleIrq(4,1);
}

void EXTI9_5_IRQHandler(void)
{
	tch_gpio_handleIrq(5,5);
}
/**
 */
void EXTI15_10_IRQHandler(void)
{
	tch_gpio_handleIrq(10,5);
}

