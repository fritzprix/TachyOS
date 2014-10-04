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



#define GPIO_Event_Signal              ((int32_t) 1)

#define GPIO_Mode_IN                   (uint8_t) (0)
#define GPIO_Mode_OUT                  (uint8_t) (1)
#define GPIO_Mode_AF                   (uint8_t) (2)
#define GPIO_Mode_AN                   (uint8_t) (3)
#define GPIO_Mode_Msk                  (GPIO_Mode_IN |\
		                                GPIO_Mode_OUT|\
		                                GPIO_Mode_AF|\
		                                GPIO_Mode_AN)

#define GPIO_Otype_PP                   (uint8_t) (0)
#define GPIO_Otype_OD                   (uint8_t) (1)
#define GPIO_Otype_Msk                  (GPIO_Otype_PP | GPIO_Otype_OD)


#define GPIO_OSpeed_2M                  (uint8_t) (0)
#define GPIO_OSpeed_25M                 (uint8_t) (1)
#define GPIO_OSpeed_50M                 (uint8_t) (2)
#define GPIO_OSpeed_100M                (uint8_t) (3)
#define GPIO_OSpeed_Msk                 (GPIO_OSpeed_2M|\
		                                 GPIO_OSpeed_25M|\
		                                 GPIO_OSpeed_50M|\
		                                 GPIO_OSpeed_100M)

#define GPIO_PuPd_Float                 (uint8_t) (0)
#define GPIO_PuPd_PU                    (uint8_t) (1)
#define GPIO_PuPd_PD                    (uint8_t) (2)
#define GPIO_PuPd_Msk                   (GPIO_PuPd_Float|\
		                                 GPIO_PuPd_PU|\
		                                 GPIO_PuPd_PD)

#define GPIO_EvEdge_Rise               ((uint8_t) 1)
#define GPIO_EvEdge_Fall               ((uint8_t) 2)
#define GPIO_EvEdge_Both               ((uint8_t) GPIO_EvEdge_Fall | GPIO_EvEdge_Rise)


#define GPIO_EvType_Interrupt          ((uint8_t) 1)
#define GPIO_EvType_Event              ((uint8_t) 2)

#define INIT_GPIOPORT_TYPE             {\
	                                     _GPIO_0,\
	                                     _GPIO_1,\
	                                     _GPIO_2,\
	                                     _GPIO_3,\
	                                     _GPIO_4,\
	                                     _GPIO_5,\
	                                     _GPIO_6,\
	                                     _GPIO_7,\
	                                     _GPIO_8,\
	                                     _GPIO_9,\
	                                     _GPIO_10,\
	                                     _GPIO_11\
}


#define INIT_GPIOMODE_TYPE             {GPIO_Mode_OUT,\
	                                    GPIO_Mode_IN,\
	                                    GPIO_Mode_AN,\
	                                    GPIO_Mode_AF}

#define INIT_GPIOOTYPE_TYPE            {GPIO_Otype_PP,\
	                                    GPIO_Otype_OD}

#define INIT_GPIOSPEED_TYPE            {GPIO_OSpeed_2M,\
	                                    GPIO_OSpeed_25M,\
	                                    GPIO_OSpeed_50M,\
	                                    GPIO_OSpeed_100M}

#define INIT_GPIOPUPD_TYPE             {GPIO_PuPd_PU,\
	                                    GPIO_PuPd_PD,\
	                                    GPIO_PuPd_Float}

#define INIT_GPIO_EVEdge_TYPE          {GPIO_EvEdge_Rise,\
	                                    GPIO_EvEdge_Fall,\
	                                    GPIO_EvEdge_Both}


#define INIT_GPIO_EVType_TYPE          {GPIO_EvType_Interrupt,\
	                                    GPIO_EvType_Event}




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
	tch*                          sys;
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
static void tch_gpio_freeIo(tch_GpioHandle* IoHandle);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/

static void tch_gpio_handle_out(tch_gpio_handle_prototype* self,uint32_t pmsk,tch_bState nstate);
static tch_bState tch_gpio_handle_in(tch_gpio_handle_prototype* self,uint32_t pmsk);
static tchStatus tch_gpio_handle_registerIoEvent(tch_gpio_handle_prototype* self,const tch_GpioEvCfg* cfg,uint32_t* pmsk);
static tchStatus tch_gpio_handle_unregisterIoEvent(tch_gpio_handle_prototype* self,uint32_t pmsk);
static tchStatus tch_gpio_handle_configureEvent(tch_gpio_handle_prototype* self,const tch_GpioEvCfg* cfg,uint32_t pmsk);
static tchStatus tch_gpio_handle_configure(tch_gpio_handle_prototype* self,const tch_GpioCfg* cfg,uint32_t pmsk);
static BOOL tch_gpio_handle_listen(tch_gpio_handle_prototype* self,uint8_t pin,uint32_t timeout);



/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle);
static void tch_gpio_setIrq(tch_gpio_handle_prototype* _handle,uint8_t pin,const tch_GpioEvCfg* cfg);
static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt);

static inline void tch_gpioValidate(tch_gpio_handle_prototype* _handle);
static inline void tch_gpioInvalidate(tch_gpio_handle_prototype* _handle);
static inline BOOL tch_gpioIsValid(tch_gpio_handle_prototype* _handle);


/**
 * 	tch_gpioMode Mode;
	tch_gpioOtype Otype;
	tch_gpioSpeed Speed;
	tch_gpioPuPd PuPd;
	tch_gpioEvEdge EvEdeg;
	tch_gpioEvType EvType;
 */

__attribute__((section(".data"))) static tch_gpio_manager GPIO_StaticManager = {

		{
				INIT_GPIOPORT_TYPE,
				INIT_GPIOMODE_TYPE,
				INIT_GPIOOTYPE_TYPE,
				INIT_GPIOSPEED_TYPE,
				INIT_GPIOPUPD_TYPE,
				INIT_GPIO_EVEdge_TYPE,
				INIT_GPIO_EVType_TYPE,
				tch_gpio_allocIo,
				tch_gpio_initCfg,
				tch_gpio_initEvCfg,
				tch_gpio_getPortCount,
				tch_gpio_getPincount,
				tch_gpio_getPinAvailable,
				tch_gpio_freeIo
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
	instance->sys = api;
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
	tch_gpio_handle_configure(instance,cfg,pmsk);
	tch_gpioValidate(instance);
	return (tch_GpioHandle*) instance;
}

static void tch_gpio_initCfg(tch_GpioCfg* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_StaticManager._pix.Mode.In;
	cfg->Speed = GPIO_StaticManager._pix.Speed.Low;
	cfg->Otype = GPIO_StaticManager._pix.Otype.PushPull;
	cfg->PuPd = GPIO_StaticManager._pix.PuPd.NoPull;
}

static void tch_gpio_initEvCfg(tch_GpioEvCfg* evcfg){
	evcfg->EvEdge = GPIO_StaticManager._pix.EvEdeg.Rise;
	evcfg->EvType = GPIO_StaticManager._pix.EvType.Interrupt;
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

static void tch_gpio_freeIo(tch_GpioHandle* IoHandle){
	tch_gpio_handle_prototype* _handle = (tch_gpio_handle_prototype*) IoHandle;
	if(!tch_gpioIsValid(_handle))
		return;
	tch* api = _handle->sys;
	if(api->Mtx->lock(_handle->mtxId,osWaitForever) != osOK)
		return;
	if(api->Mtx->lock(GPIO_StaticManager.mtxId,osWaitForever) != osOK)
		return;
	tch_gpio_descriptor* gpio = &GPIO_HWs[_handle->idx];
	tch_GpioCfg initCfg;
	tch_gpio_initCfg(&initCfg);
	tch_gpio_handle_configure(_handle,&initCfg,_handle->pMsk);

	*gpio->_clkenr &= ~gpio->clkmsk;
	*gpio->_lpclkenr &= ~gpio->lpclkmsk;

	gpio->io_ocpstate &= ~_handle->pMsk;
	api->Condv->wakeAll(GPIO_StaticManager.condvId);

	_handle->sys->Mtx->destroy(_handle->mtxId);
	api->Mem->free(_handle);
	tch_gpioInvalidate(_handle);
	api->Mtx->unlock(GPIO_StaticManager.mtxId);
}



static void tch_gpio_handle_out(tch_gpio_handle_prototype* _handle,uint32_t pmsk,tch_bState nstate){
	if(!tch_gpioIsValid(_handle))
		return;
	if(!(_handle->pMsk & pmsk))
		return;
	if(nstate){
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR |= pmsk;
	}else{
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR &= ~pmsk;
	}
}

static tch_bState tch_gpio_handle_in(tch_gpio_handle_prototype* _handle,uint32_t pmsk){
	if(!tch_gpioIsValid(_handle))
		return bClear;
	if(!(_handle->pMsk & pmsk))
		return bClear;
	return ((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->IDR & pmsk ? bSet : bClear;
}

/**
 */
static tchStatus tch_gpio_handle_registerIoEvent(tch_gpio_handle_prototype* _handle,const tch_GpioEvCfg* cfg,uint32_t* pmsk){
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!(_handle->pMsk & *pmsk))
		return osErrorParameter;
	if(__get_IPSR())
		return osErrorISR;
	uint8_t pin = 0;
	tch_ioInterrupt_descriptor* ioIrqObj = NULL;
	tch* api = _handle->sys;
	uint32_t p_msk = *pmsk;
	while(p_msk){
		if(p_msk & 0x01){
			ioIrqObj = &IoInterrupt_HWs[pin];
			if(api->Mtx->lock(GPIO_StaticManager.mtxId,cfg->EvTimeout) != osOK){
				*pmsk = (1 << pin);
				return osErrorResource;
			}
			if(!ioIrqObj->condv)
				ioIrqObj->condv = api->Condv->create();
			if(!ioIrqObj->evbar)
				ioIrqObj->evbar = api->Barrier->create();
			while(ioIrqObj->io_occp){
				if(api->Condv->wait(ioIrqObj->condv,GPIO_StaticManager.mtxId,cfg->EvTimeout) != osOK)
					*pmsk = (1 << pin);
					return osErrorResource;
			}
			ioIrqObj->io_occp = _handle;
			SET_INTERRUPT(_handle);
			RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
			_handle->cb = cfg->EvCallback;
			tch_gpio_setIrq(_handle,pin,cfg);
			api->Mtx->unlock(GPIO_StaticManager.mtxId);
			*pmsk &= ~(1 << pin);
		}
		pin++;
		p_msk >>= 1;
	}
	return osOK;
}

static tchStatus tch_gpio_handle_unregisterIoEvent(tch_gpio_handle_prototype* _handle,uint32_t pmsk){
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!IS_INTERRUPT(_handle))
		return osErrorParameter;
	if(__get_IPSR()){
		return osErrorISR;
	}
	tch_ioInterrupt_descriptor* ioIrqObj = NULL;
	uint8_t pin = 0;
	uint32_t cmsk = 1;               // declare bit mask to clear interrupt config register
	tch* api = _handle->sys;

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

static tchStatus tch_gpio_handle_configureEvent(tch_gpio_handle_prototype* self,const tch_GpioEvCfg* cfg,uint32_t pmsk){

}


static tchStatus tch_gpio_handle_configure(tch_gpio_handle_prototype* _handle,const tch_GpioCfg* cfg,uint32_t pmsk){
	if(!tch_gpioIsValid(_handle))
		return osErrorResource;
	if(!(_handle->pMsk & pmsk))
		return osErrorParameter;
	GPIO_TypeDef* ioctrl_regs = (GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw;
	uint8_t pin = 0;
	uint32_t p_msk = 1;
	while(pmsk){
		if(pmsk & 0x01){
			ioctrl_regs->MODER &= ~(GPIO_Mode_Msk << pin);
			switch(cfg->Mode){
			case GPIO_Mode_OUT:             /// gpio configure to output
				ioctrl_regs->MODER |= (GPIO_Mode_OUT << (pin << 1));

				ioctrl_regs->OTYPER &= ~(GPIO_Otype_Msk << pin);
				if(cfg->Otype == GPIO_Otype_OD){
					ioctrl_regs->OTYPER |= (1 << pin);
				}else{
					ioctrl_regs->OTYPER &= ~(0 << pin);
				}
				ioctrl_regs->OSPEEDR &= ~(GPIO_OSpeed_Msk << (pin << 1));
				ioctrl_regs->OSPEEDR |= (cfg->Speed << (pin << 1));
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

				ioctrl_regs->AFR[pin / 8] &= ~(0xF << (pin << 2));
				ioctrl_regs->AFR[pin / 8] |= (cfg->Af << (pin << 2));
			}
		}
		pin++;
		pmsk >>= 1;
	}
	return osOK;
}

static BOOL tch_gpio_handle_listen(tch_gpio_handle_prototype* _handle,uint8_t pin,uint32_t timeout){
	if(!tch_gpioIsValid(_handle))
		return FALSE;
	if(!IS_INTERRUPT(_handle))
		return FALSE;
	if(__get_IPSR())
		return FALSE;
	uint32_t pmsk = (1 << pin);
	if(!(_handle->pMsk & pmsk))
		return FALSE;
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[pin];
	tch* api = _handle->sys;
	return api->Barrier->wait(ioIrqObj->evbar,timeout) == osOK;
}


static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle){
	handle->_pix.in = tch_gpio_handle_in;
	handle->_pix.out = tch_gpio_handle_out;
	handle->_pix.listen = tch_gpio_handle_listen;
	handle->_pix.registerIoEvent = tch_gpio_handle_registerIoEvent;
	handle->_pix.unregisterIoEvent = tch_gpio_handle_unregisterIoEvent;
	handle->_pix.configure = tch_gpio_handle_configure;
	handle->_pix.configureEvent = tch_gpio_handle_configureEvent;
	handle->pMsk = 0;
	handle->mtxId = handle->sys->Mtx->create();
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
	tch_lld_intr* intr = _handle->sys->Device->interrupt;
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
			_handle->sys->Barrier->signal(ioIntObj->evbar,osOK);
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

