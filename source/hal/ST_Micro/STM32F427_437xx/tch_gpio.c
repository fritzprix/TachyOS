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
	tch_lld_gpio                    _pix;
	tch_mtxId                        mtxId;
	tch_condvId                      condvId;
	const uint8_t                    port_cnt;
	const uint8_t                    io_cnt;
}tch_gpio_manager;

typedef struct _tch_gpio_handle_prototype {
	tch_GpioHandle               _pix;
	uint8_t                       idx;
	uint32_t                      state;
	uint32_t                      pMsk;
	tch_mtxId                     mtxId;
	tch_IoEventCallback_t         cb;
	const tch*                          env;
}tch_gpio_handle_prototype;


/////////////////////////////////////    public function     //////////////////////////////////


/**********************************************************************************************/
/************************************  gpio driver public function  ***************************/
/**********************************************************************************************/
static tch_GpioHandle* tch_gpio_allocIo(const tch* api,const gpIo_x port,uint32_t pmsk,const tch_GpioCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg);
static void tch_gpio_initCfg(tch_GpioCfg* cfg);
static void tch_gpio_initEvCfg(tch_GpioEvCfg* evcfg);
static uint16_t tch_gpio_getPortCount();
static uint16_t tch_gpio_getPincount(const gpIo_x port);
static uint32_t tch_gpio_getPinAvailable(const gpIo_x port);
static tchStatus tch_gpio_handle_freeIo(tch_GpioHandle* self);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/

static void tch_gpio_handle_out(tch_GpioHandle* self,uint32_t pmsk,tch_bState nstate);
static tch_bState tch_gpio_handle_in(tch_GpioHandle* self,uint32_t pmsk);
static tchStatus tch_gpio_handle_registerIoEvent(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t* pmsk);
static tchStatus tch_gpio_handle_unregisterIoEvent(tch_GpioHandle* self,uint32_t pmsk);
static tchStatus tch_gpio_handle_configureEvent(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t pmsk);
static tchStatus tch_gpio_handle_configure(tch_GpioHandle* self,const tch_GpioCfg* cfg,uint32_t pmsk);
static BOOL tch_gpio_handle_listen(tch_GpioHandle* self,uint8_t pin,uint32_t timeout);



/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle);
static void tch_gpio_setIrq(tch_gpio_handle_prototype* _handle,uint8_t pin,const tch_GpioEvCfg* cfg);
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
				tch_gpio_getPortCount,
				tch_gpio_getPincount,
				tch_gpio_getPinAvailable
		},
		NULL,
		NULL,
		MFEATURE_GPIO,
		MFEATURE_PINCOUNT_pPort
};

const tch_lld_gpio* tch_gpio_instance = (tch_lld_gpio*) &GPIO_StaticManager;



static tch_GpioHandle* tch_gpio_allocIo(const tch* api,const gpIo_x port,uint32_t pmsk,const tch_GpioCfg* cfg,uint32_t timeout,tch_PwrOpt pcfg){
	tch_gpio_descriptor* gpio = &GPIO_HWs[port];
	if(!gpio->_clkenr){                     /// given GPIO port is not supported in this H/W
		return NULL;
	}

	if(!GPIO_StaticManager.mtxId)
		GPIO_StaticManager.mtxId = Mtx->create();
	if(!GPIO_StaticManager.condvId)
		GPIO_StaticManager.condvId = Condv->create();
	if(api->Mtx->lock(GPIO_StaticManager.mtxId,timeout) != osOK)
		return NULL;
	while(gpio->io_ocpstate & pmsk){           /// if there is pin which is occupied by another instance
		if(api->Condv->wait(GPIO_StaticManager.condvId,GPIO_StaticManager.mtxId,timeout) != osOK)
			return NULL;
	}
	gpio->io_ocpstate |= pmsk;     // mark pin as occupied
	api->Mtx->unlock(GPIO_StaticManager.mtxId);

	tch_gpio_handle_prototype* instance = api->Mem->alloc(sizeof(tch_gpio_handle_prototype));   // allocate gpio handle
	if(!instance)
		return NULL;
	instance->env = api;
	tch_gpio_initGpioHandle(instance);
	instance->pMsk = pmsk;
	instance->idx = port;

	GPIO_TypeDef* ioctrl_regs = (GPIO_TypeDef*) gpio->_hw;

	*gpio->_clkenr |= gpio->clkmsk;              /// enable gpio clock

	if(pcfg == ActOnSleep){                      /// if active-on-sleep is enabled
		*gpio->_lpclkenr |= gpio->lpclkmsk;      /// set lp clk bit
	}else{
		*gpio->_lpclkenr &= ~gpio->lpclkmsk;     /// otherwise clear
	}
	tch_gpio_handle_configure((tch_GpioHandle*) instance,cfg,pmsk);
	tch_gpioValidate(instance);
	return (tch_GpioHandle*) instance;
}

static void tch_gpio_initCfg(tch_GpioCfg* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_Mode_IN;
	cfg->Speed = GPIO_OSpeed_2M;
	cfg->Otype = GPIO_Otype_OD;
	cfg->PuPd = GPIO_PuPd_Float;
}

static void tch_gpio_initEvCfg(tch_GpioEvCfg* evcfg){
	evcfg->EvEdge = GPIO_EvEdge_Fall;
	evcfg->EvType = GPIO_EvType_Interrupt;
	evcfg->EvCallback = NULL;
	evcfg->EvTimeout = osWaitForever;
}


static uint16_t tch_gpio_getPortCount(){
	return GPIO_StaticManager.port_cnt;
}

static uint16_t tch_gpio_getPincount(const gpIo_x port){
	return GPIO_StaticManager.io_cnt;
}

static uint32_t tch_gpio_getPinAvailable(const gpIo_x port){
	return GPIO_HWs[port].io_ocpstate;
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
	env->Mem->free(_handle);
	env->Mtx->unlock(GPIO_StaticManager.mtxId);
	return osOK;
}



static void tch_gpio_handle_out(tch_GpioHandle* self,uint32_t pmsk,tch_bState nstate){
	if(!self)
		return;
	tch_gpio_handle_prototype* _handle = NULL;
	if(!tch_gpioIsValid(_handle))
		return;
	 _handle = (tch_gpio_handle_prototype*) self;
	if(!(_handle->pMsk & pmsk))
		return;
	if(nstate){
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR |= pmsk;
	}else{
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR &= ~pmsk;
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

static tchStatus tch_gpio_handle_registerIoEvent(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t* pmsk){
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return osErrorParameter;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!(_handle->pMsk & *pmsk))
		return osErrorParameter;
	if(__get_IPSR())
		return osErrorISR;
	uint8_t pin = 0;
	tch_ioInterrupt_descriptor* ioIrqObj = NULL;
	const tch* env = _handle->env;
	uint32_t p_msk = *pmsk;
	while(p_msk){
		if(p_msk & 0x01){
			ioIrqObj = &IoInterrupt_HWs[pin];
			if(env->Mtx->lock(GPIO_StaticManager.mtxId,cfg->EvTimeout) != osOK){
				*pmsk = (1 << pin);
				return osErrorResource;
			}
			if(!ioIrqObj->condv)
				ioIrqObj->condv = env->Condv->create();
			if(!ioIrqObj->evbar)
				ioIrqObj->evbar = env->Barrier->create();
			while(ioIrqObj->io_occp){
				if(env->Condv->wait(ioIrqObj->condv,GPIO_StaticManager.mtxId,cfg->EvTimeout) != osOK)
					*pmsk = (1 << pin);
					return osErrorResource;
			}
			ioIrqObj->io_occp = _handle;
			SET_INTERRUPT(_handle);
			RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
			_handle->cb = cfg->EvCallback;
			tch_gpio_setIrq(_handle,pin,cfg);
			env->Mtx->unlock(GPIO_StaticManager.mtxId);
			*pmsk &= ~(1 << pin);
		}
		pin++;
		p_msk >>= 1;
	}
	return osOK;
}

static tchStatus tch_gpio_handle_unregisterIoEvent(tch_GpioHandle* self,uint32_t pmsk){
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return osErrorParameter;
	_handle = (tch_gpio_handle_prototype* )self;
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!IS_INTERRUPT(_handle))
		return osErrorParameter;
	if(__get_IPSR())
		return osErrorISR;
	tch_ioInterrupt_descriptor* ioIrqObj = NULL;
	uint8_t pin = 0;
	uint32_t cmsk = 1;               // declare bit mask to clear interrupt config register
	const tch* api = _handle->env;

	if(api->Mtx->lock(GPIO_StaticManager.mtxId,osWaitForever) != osOK)   // lock mtx of singleton
		return osErrorResource;
	while(pmsk){
		if(pmsk & 0x01){
			ioIrqObj = &IoInterrupt_HWs[pin];
			ioIrqObj->io_occp = NULL;
			api->Device->interrupt->disable(ioIrqObj->irq);
			SYSCFG->EXTICR[pin >> 2] = 0;
			EXTI->EMR &= ~cmsk;
			EXTI->IMR &= ~cmsk;
			EXTI->FTSR &= ~cmsk;
			EXTI->RTSR &= ~cmsk;
			api->Barrier->signal(ioIrqObj->evbar,osErrorResource);
			api->Condv->wakeAll(ioIrqObj->condv);   // wake up thread wating for interrupt allocation
		}
		cmsk <<= 1;
		pin++;
		pmsk >>= 1;
	}

	_handle->cb = NULL;
	CLR_INTERRUPT(_handle);    // clear interrupt flag in status
	api->Mtx->unlock(GPIO_StaticManager.mtxId);   // release mtx of singleton

	return osOK;

}

static tchStatus tch_gpio_handle_configureEvent(tch_GpioHandle* self,const tch_GpioEvCfg* cfg,uint32_t pmsk){
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return osErrorParameter;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	const tch* env = ((tch_gpio_handle_prototype*)self)->env;
	return osOK;
}


static tchStatus tch_gpio_handle_configure(tch_GpioHandle* self,const tch_GpioCfg* cfg,uint32_t pmsk){
	tch_gpio_handle_prototype* _handle = NULL;
	GPIO_TypeDef* ioctrl_regs = NULL;
	uint8_t pin = 0;
	uint32_t p_msk = 1;

	if(!self)
		return osErrorParameter;
	 _handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!(_handle->pMsk & pmsk))
		return osErrorParameter;
	ioctrl_regs = (GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw;
	while(pmsk){
		if(pmsk & 0x01){
			ioctrl_regs->MODER &= ~(GPIO_Mode_Msk << (pin << 1));
			ioctrl_regs->OSPEEDR &= ~(GPIO_OSpeed_Msk << (pin << 1));
			ioctrl_regs->OSPEEDR |= (cfg->Speed << (pin << 1));
			switch(cfg->Mode){
			case GPIO_Mode_OUT:             /// gpio configure to output
				ioctrl_regs->MODER |= (GPIO_Mode_OUT << (pin << 1));

				ioctrl_regs->OTYPER &= ~(GPIO_Otype_Msk << pin);
				if(cfg->Otype == GPIO_Otype_OD){
					ioctrl_regs->OTYPER |= (1 << pin);
				}else{
					ioctrl_regs->OTYPER &= ~(0 << pin);
				}
				break;
			case GPIO_Mode_IN:
				ioctrl_regs->MODER |= (GPIO_Mode_IN << (pin << 1));

				ioctrl_regs->PUPDR &= ~(GPIO_PuPd_Msk << (pin << 1));
				ioctrl_regs->PUPDR |= (cfg->PuPd << (pin << 1));
				break;
			case GPIO_Mode_AN:
				ioctrl_regs->MODER |= (GPIO_Mode_AN << (pin  << 1));
				break;
			case GPIO_Mode_AF:
				ioctrl_regs->MODER |= (GPIO_Mode_AF << (pin << 1));
				if(pin < 8){
					ioctrl_regs->AFR[0] &= ~(0xF << (pin << 2));
					ioctrl_regs->AFR[0] |= (cfg->Af << (pin << 2));
				}else{
					ioctrl_regs->AFR[1] &= ~(0xF << ((pin - 8) << 2));
					ioctrl_regs->AFR[1] |= (cfg->Af << ((pin - 8) << 2));
				}
			}
		}
		pin++;
		pmsk >>= 1;
	}
	return osOK;
}

static BOOL tch_gpio_handle_listen(tch_GpioHandle* self,uint8_t pin,uint32_t timeout){
	tch_gpio_handle_prototype* _handle = NULL;
	if(!self)
		return FALSE;
	_handle = (tch_gpio_handle_prototype*) self;
	if(!tch_gpioIsValid(_handle))
		return FALSE;
	if(!IS_INTERRUPT(_handle))
		return FALSE;
	if(__get_IPSR())
		return FALSE;
	uint32_t pmsk = (1 << pin);
	if(!(_handle->pMsk & pmsk))
		return FALSE;
	const tch* env = _handle->env;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[pin];
	return env->Barrier->wait(ioIrqObj->evbar,timeout) == osOK;
}


static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle){
	handle->_pix.close = tch_gpio_handle_freeIo;
	handle->_pix.in = tch_gpio_handle_in;
	handle->_pix.out = tch_gpio_handle_out;
	handle->_pix.listen = tch_gpio_handle_listen;
	handle->_pix.registerIoEvent = tch_gpio_handle_registerIoEvent;
	handle->_pix.unregisterIoEvent = tch_gpio_handle_unregisterIoEvent;
	handle->_pix.configure = tch_gpio_handle_configure;
	handle->_pix.configureEvent = tch_gpio_handle_configureEvent;
	handle->pMsk = 0;
	handle->mtxId = handle->env->Mtx->create();
	handle->cb = NULL;
}


static void tch_gpio_setIrq(tch_gpio_handle_prototype* _handle,uint8_t pin,const tch_GpioEvCfg* cfg){
	tch_ioInterrupt_descriptor* ioIntObj = &IoInterrupt_HWs[pin];
	EXTI->RTSR &= ~_handle->pMsk;
	EXTI->FTSR &= ~_handle->pMsk;

	if(cfg->EvEdge & GPIO_EvEdge_Rise){
		EXTI->RTSR |= _handle->pMsk;
	}
	if(cfg->EvEdge & GPIO_EvEdge_Fall){
		EXTI->FTSR |= _handle->pMsk;
	}

	EXTI->EMR &= ~_handle->pMsk;
	EXTI->IMR &= ~_handle->pMsk;
	if(cfg->EvType & GPIO_EvType_Event){
		EXTI->EMR |= _handle->pMsk;
	}
	if(cfg->EvType & GPIO_EvType_Interrupt){
		EXTI->IMR |= _handle->pMsk;
	}
	const tch_lld_intr* intr = _handle->env->Device->interrupt;
	intr->setPriority(ioIntObj->irq,intr->Priority.Normal);
	intr->enable(ioIntObj->irq);
	SYSCFG->EXTICR[pin >> 2] |= _handle->idx << ((pin % 4) *4);
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
