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
#include "tch.h"
#include "tch_halInit.h"
#include "tch_gpio.h"
#include "tch_ktypes.h"
#include "tch_kernel.h"
#include "tch_halcfg.h"



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
#define TCH_GPIO_CLASS_KEY       ((uint16_t) 0x3D03)


#define SET_INTERRUPT(gpio_handle)  ((tch_gpio_handle_prototype*) gpio_handle)->state |= GPIO_INTERRUPT_INIT
#define CLR_INTERRUPT(gpio_handle)  ((tch_gpio_handle_prototype*) gpio_handle)->state &= ~GPIO_INTERRUPT_INIT_MSK
#define IS_INTERRUPT(gpio_handle)   ((((tch_gpio_handle_prototype*) gpio_handle)->state & GPIO_INTERRUPT_INIT_MSK) == GPIO_INTERRUPT_INIT)


typedef struct tch_gpio_manager_internal_t {
	tch_lld_gpio                 _pix;
	tch_mtxId                     mtxId;
	tch_condvId                   condvId;
	const uint8_t                 port_cnt;
	const uint8_t                 io_cnt;
}tch_gpio_manager;

typedef struct _tch_gpio_handle_prototype {
	tch_GpioHandle               _pix;
	uint8_t                       idx;
	uint32_t                      state;
	uint32_t                      pMsk;
	tch_mtxId                     mtxId;
	tch_IoEventCallback_t         cb;
	const tch*                    env;
}tch_gpio_handle_prototype;


/////////////////////////////////////    public function     //////////////////////////////////


/**********************************************************************************************/
/************************************  gpio driver public function  ***************************/
/**********************************************************************************************/
static tch_GpioHandle* tch_gpio_allocIo(const tch* env,const gpIo_x port,uint32_t pmsk,const tch_GpioCfg* cfg,uint32_t timeout);
static void tch_gpio_initCfg(tch_GpioCfg* cfg);
static void tch_gpio_initEvCfg(tch_GpioEvCfg* evcfg);
static tchStatus tch_gpio_handle_freeIo(tch_GpioHandle* self);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/

static void tch_gpio_handle_out(tch_GpioHandle* self,uint32_t pmsk,tch_bState nstate);
static tch_bState tch_gpio_handle_in(tch_GpioHandle* self,uint32_t pmsk);
static tchStatus tch_gpio_handle_registerIoEvent(tch_GpioHandle* self,pin p,const tch_GpioEvCfg* cfg);
static tchStatus tch_gpio_handle_unregisterIoEvent(tch_GpioHandle* self,pin p);
static tchStatus tch_gpio_handle_configureEvent(tch_GpioHandle* self,pin p,const tch_GpioEvCfg* cfg);
static tchStatus tch_gpio_handle_configure(tch_GpioHandle* self,const tch_GpioCfg* cfg,uint32_t pmsk);
static BOOL tch_gpio_handle_listen(tch_GpioHandle* self,uint8_t pin,uint32_t timeout);



/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt);

static inline void tch_gpioValidate(tch_gpio_handle_prototype* _handle);
static inline void tch_gpioInvalidate(tch_gpio_handle_prototype* _handle);
static inline BOOL tch_gpioIsValid(tch_gpio_handle_prototype* _handle);


__attribute__((section(".data"))) static tch_gpio_manager GPIO_StaticManager = {

		{
				MFEATURE_GPIO,
				tch_gpio_allocIo,
				tch_gpio_initCfg,
				tch_gpio_initEvCfg,
		},
		NULL,
		NULL,
		MFEATURE_GPIO,
		MFEATURE_PINCOUNT_pPort
};

const tch_lld_gpio* tch_gpio_instance = (tch_lld_gpio*) &GPIO_StaticManager;



static tch_GpioHandle* tch_gpio_allocIo(const tch* env,const gpIo_x port,uint32_t pmsk,const tch_GpioCfg* cfg,uint32_t timeout){
	tch_gpio_descriptor* gpio = &GPIO_HWs[port];
	if(!gpio->_clkenr)                   /// given GPIO port is not supported in this H/W
		return NULL;
	if(!GPIO_StaticManager.mtxId)
		GPIO_StaticManager.mtxId = Mtx->create();
	if(!GPIO_StaticManager.condvId)
		GPIO_StaticManager.condvId = Condv->create();
	if(env->Mtx->lock(GPIO_StaticManager.mtxId,timeout) != osOK)
		return NULL;
	while(gpio->io_ocpstate & pmsk){           /// if there is pin which is occupied by another instance
		if(env->Condv->wait(GPIO_StaticManager.condvId,GPIO_StaticManager.mtxId,timeout) != osOK)
			return NULL;
	}
	gpio->io_ocpstate |= pmsk;     // mark pin as occupied
	env->Mtx->unlock(GPIO_StaticManager.mtxId);

	tch_gpio_handle_prototype* ins = env->Mem->alloc(sizeof(tch_gpio_handle_prototype));   // allocate gpio handle
	ins->env = env;
	ins->_pix.close = tch_gpio_handle_freeIo;
	ins->_pix.in = tch_gpio_handle_in;
	ins->_pix.out = tch_gpio_handle_out;
	ins->_pix.listen = tch_gpio_handle_listen;
	ins->_pix.registerIoEvent = tch_gpio_handle_registerIoEvent;
	ins->_pix.unregisterIoEvent = tch_gpio_handle_unregisterIoEvent;
	ins->_pix.configure = tch_gpio_handle_configure;
	ins->_pix.configureEvent = tch_gpio_handle_configureEvent;
	ins->pMsk = pmsk;
	ins->mtxId = env->Mtx->create();
	ins->cb = NULL;
	ins->idx = port;

	*gpio->_clkenr |= gpio->clkmsk;              /// enable gpio clock
	if(cfg->popt == ActOnSleep){                      /// if active-on-sleep is enabled
		*gpio->_lpclkenr |= gpio->lpclkmsk;      /// set lp clk bit
	}else{
		*gpio->_lpclkenr &= ~gpio->lpclkmsk;     /// otherwise clear
	}
	tch_gpio_handle_configure((tch_GpioHandle*) ins,cfg,pmsk);
	tch_gpioValidate(ins);
	return (tch_GpioHandle*) ins;
}

static void tch_gpio_initCfg(tch_GpioCfg* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_Mode_IN;
	cfg->Speed = GPIO_OSpeed_2M;
	cfg->Otype = GPIO_Otype_OD;
	cfg->PuPd = GPIO_PuPd_Float;
	cfg->popt = ActOnSleep;
}

static void tch_gpio_initEvCfg(tch_GpioEvCfg* evcfg){
	evcfg->EvEdge = GPIO_EvEdge_Fall;
	evcfg->EvType = GPIO_EvType_Interrupt;
	evcfg->EvCallback = NULL;
	evcfg->EvTimeout = osWaitForever;
}


static tchStatus tch_gpio_handle_freeIo(tch_GpioHandle* self){
	tch_gpio_handle_prototype* _handle = NULL;
	tchStatus result = 0;
	if(!self)
		return osErrorParameter;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	const tch* env = _handle->env;
	if(env->Mtx->lock(_handle->mtxId,osWaitForever) != osOK)
		return osErrorTimeoutResource;
	tch_gpioInvalidate(_handle);
	env->Mtx->destroy(_handle->mtxId);

	if((result = env->Mtx->lock(GPIO_StaticManager.mtxId,osWaitForever)) != osOK)
		return result;
	tch_gpio_descriptor* gpio = &GPIO_HWs[_handle->idx];
	tch_GpioCfg initCfg;
	tch_gpio_initCfg(&initCfg);
	tch_gpio_handle_configure(self,&initCfg,_handle->pMsk);
	*gpio->_clkenr &= ~gpio->clkmsk;
	*gpio->_lpclkenr &= ~gpio->lpclkmsk;


	gpio->io_ocpstate &= ~_handle->pMsk;
	env->Condv->wakeAll(GPIO_StaticManager.condvId);
	tch_gpioInvalidate(_handle);
	env->Mtx->unlock(GPIO_StaticManager.mtxId);
	env->Mem->free(_handle);
	return osOK;
}



static void tch_gpio_handle_out(tch_GpioHandle* self,uint32_t pmsk,tch_bState nstate){
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if(!ins)
		return;
	if(!tch_gpioIsValid(ins))
		return;
	if(!(ins->pMsk & pmsk))
		return;
	if(nstate){
		((GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw)->ODR |= pmsk;
	}else{
		((GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw)->ODR &= ~pmsk;
	}
}

static tch_bState tch_gpio_handle_in(tch_GpioHandle* self,uint32_t pmsk){
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return bClear;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return bClear;
	if(!(_handle->pMsk & pmsk))
		return bClear;
	return ((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->IDR & pmsk ? bSet : bClear;
}

static tchStatus tch_gpio_handle_registerIoEvent(tch_GpioHandle* self,pin p,const tch_GpioEvCfg* cfg){
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[p];

	if(!self)
		return osErrorParameter;
	if(!tch_gpioIsValid(ins))
		return osErrorResource;

	if(__get_IPSR())
		return osErrorISR;


	const tch* env = ins->env;
	if(env->Mtx->lock(GPIO_StaticManager.mtxId,cfg->EvTimeout) != osOK){
		return osErrorResource;
	}

	if(ioIrqObj->io_occp)
		return osErrorResource;
	ioIrqObj->evbar = env->Barrier->create();

	ioIrqObj->io_occp = ins;
	SET_INTERRUPT(ins);
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	ins->cb = cfg->EvCallback;
	tch_gpio_handle_configureEvent(self,p,cfg);
	env->Mtx->unlock(GPIO_StaticManager.mtxId);
	return osOK;
}

static tchStatus tch_gpio_handle_unregisterIoEvent(tch_GpioHandle* self,pin p){
	tch_gpio_handle_prototype* ins = NULL;
	if(!self)
		return osErrorParameter;
	ins = (tch_gpio_handle_prototype* )self;
	if(!tch_gpioIsValid(ins))
		return osErrorResource;
	if(!IS_INTERRUPT(ins))
		return osErrorParameter;
	if(__get_IPSR())
		return osErrorISR;
	tch_ioInterrupt_descriptor* ioIntrDesc = NULL;
	const tch* api = ins->env;

	if(api->Mtx->lock(GPIO_StaticManager.mtxId,osWaitForever) != osOK)   // lock mtx of singleton
		return osErrorResource;

	uint16_t pmsk = (1 << p);
	ioIntrDesc = &IoInterrupt_HWs[p];
	ioIntrDesc->io_occp = NULL;
	api->Barrier->destroy(ioIntrDesc->evbar);
	NVIC_DisableIRQ(ioIntrDesc->irq);
	SYSCFG->EXTICR[p >> 2] = 0;
	EXTI->EMR &= ~pmsk;
	EXTI->IMR &= ~pmsk;
	EXTI->FTSR &= ~pmsk;
	EXTI->RTSR &= ~pmsk;
	api->Barrier->signal(ioIntrDesc->evbar,osErrorResource);
	ins->cb = NULL;
	CLR_INTERRUPT(ins);    // clear interrupt flag in status
	api->Mtx->unlock(GPIO_StaticManager.mtxId);   // release mtx of singleton

	return osOK;

}


static tchStatus tch_gpio_handle_configure(tch_GpioHandle* self,const tch_GpioCfg* cfg,uint32_t pmsk){
	tch_gpio_handle_prototype* ins = NULL;
	GPIO_TypeDef* ioHw = NULL;
	uint8_t pin = 0;
	uint32_t p_msk = 1;

	if(!self)
		return osErrorParameter;
	 ins = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(ins))
		return osErrorResource;
	if(!(ins->pMsk & pmsk))
		return osErrorParameter;
	ioHw = (GPIO_TypeDef*)GPIO_HWs[ins->idx]._hw;
	while(pmsk){
		if(pmsk & 0x01){
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
	return osOK;
}

static BOOL tch_gpio_handle_listen(tch_GpioHandle* self,uint8_t pin,uint32_t timeout){
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if((!self) || (!tch_gpioIsValid(ins)) || (__get_IPSR()))
		return FALSE;
	uint32_t pmsk = (1 << pin);
	if(!(ins->pMsk & pmsk))
		return FALSE;
	const tch* env = ins->env;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[pin];
	if(ioIrqObj->io_occp != self)
		return FALSE;
	return env->Barrier->wait(ioIrqObj->evbar,timeout) == osOK;
}



static tchStatus tch_gpio_handle_configureEvent(tch_GpioHandle* self,uint8_t pin,const tch_GpioEvCfg* cfg){
	tch_gpio_handle_prototype* ins = (tch_gpio_handle_prototype*) self;
	if((!self) || (!tch_gpioIsValid(ins)) || (__get_IPSR()))
		return osErrorParameter;
	uint16_t pmsk = (1 << pin);
	if(!(ins->pMsk & pmsk))
		return osErrorParameter;
	tch_ioInterrupt_descriptor* ioDesc = &IoInterrupt_HWs[pin];

	NVIC_DisableIRQ(ioDesc->irq);
	EXTI->RTSR &= ~pmsk;
	EXTI->FTSR &= ~pmsk;

	if(cfg->EvEdge & GPIO_EvEdge_Rise){
		EXTI->RTSR |= pmsk;
	}
	if(cfg->EvEdge & GPIO_EvEdge_Fall){
		EXTI->FTSR |= pmsk;
	}

	EXTI->EMR &= ~pmsk;
	EXTI->IMR &= ~pmsk;
	if(cfg->EvType & GPIO_EvType_Event){
		EXTI->EMR |= pmsk;
	}
	if(cfg->EvType & GPIO_EvType_Interrupt){
		EXTI->IMR |= pmsk;
	}
	NVIC_SetPriority(ioDesc->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(ioDesc->irq);
	SYSCFG->EXTICR[pin >> 2] |= ins->idx << ((pin % 4) *4);
	return osOK;
}


static inline void tch_gpioValidate(tch_gpio_handle_prototype* _handle){
	_handle->state = ((uint32_t) _handle & 0xFFFF) ^ TCH_GPIO_CLASS_KEY;
}

static inline void tch_gpioInvalidate(tch_gpio_handle_prototype* _handle){
	_handle->state &= ~(0xFFFF);
}

static inline BOOL tch_gpioIsValid(tch_gpio_handle_prototype* _handle){
	return (_handle->state & 0xFFFF) == ((uint32_t) _handle & 0xFFFF) ^ TCH_GPIO_CLASS_KEY;
}

static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt){
	uint32_t ext_pr = (EXTI->PR >> base_idx);
	tch_ioInterrupt_descriptor* ioIntObj = NULL;
	tch_gpio_handle_prototype* _handle = NULL;
	uint8_t pos = 0;
	uint32_t pMsk = 1;
	while(ext_pr){
		if(ext_pr & 1){
			ioIntObj = &IoInterrupt_HWs[base_idx + pos];
			if(!ioIntObj->io_occp)
				return;
			_handle = (tch_gpio_handle_prototype*)ioIntObj->io_occp;
			if(_handle->cb)
				_handle->cb((tch_GpioHandle*)_handle,base_idx + pos);
			EXTI->PR |= pMsk;
			_handle->env->Barrier->signal(ioIntObj->evbar,osOK);
		}
		ext_pr >>= 1;
		pMsk <<= 1;
		if(!(++pos < group_cnt))
			return;
	}
}


void EXTI0_IRQHandler(void){
	tch_gpio_handleIrq(0,1);
}

void EXTI1_IRQHandler(void){
	tch_gpio_handleIrq(1,1);
}

void EXTI2_IRQHandler(void){
	tch_gpio_handleIrq(2,1);
}

void EXTI3_IRQHandler(void){
	tch_gpio_handleIrq(3,1);
}

void EXTI4_IRQHandler(void){
	tch_gpio_handleIrq(4,1);
}

/**
 */
void EXTI9_5_IRQHandler(void){
	tch_gpio_handleIrq(5,5);
}
/**
 */
void EXTI15_10_IRQHandler(void){
	tch_gpio_handleIrq(10,5);
}

