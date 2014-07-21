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

#include "tch.h"
#include "tch_kernel.h"
#include "tch_halobjs.h"
#include "hal/tch_gpio.h"



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



typedef struct tch_gpio_manager_internal_t {
	tch_lld_gpio                    _pix;
	const uint8_t                    port_cnt;
	const uint8_t                    io_cnt;
}tch_gpio_manager;

typedef struct _tch_gpio_handle_prototype {
	tch_gpio_handle              _pix;
	uint8_t                       idx;
	uint8_t                       pin;
	uint32_t                      pMsk;
	tch_mtx_id                    mtx;
	tch_mtx                       mtx_hdr;
	tch_genericList_queue_t       wq;
	tch_IoEventCalback_t          cb;
}tch_gpio_handle_prototype;


/////////////////////////////////////    public function     //////////////////////////////////


/**********************************************************************************************/
/************************************  gpio driver public function  ***************************/
/**********************************************************************************************/
static tch_gpio_handle* tch_gpio_allocIo(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,tch_pwr_def pcfg);
static void tch_gpio_initCfg(tch_gpio_cfg* cfg);
static void tch_gpio_initEvCfg(tch_gpio_evCfg* evcfg);
static uint16_t tch_gpio_getPortCount();
static uint16_t tch_gpio_getPincount(const gpIo_x port);
static uint32_t tch_gpio_getPinAvailable(const gpIo_x port);
static void tch_gpio_freeIo(tch_gpio_handle* IoHandle);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/
static void tch_gpio_handle_out(tch_gpio_handle_prototype* self,tch_bState nstate);
static tch_bState tch_gpio_handle_in(tch_gpio_handle_prototype* self);
static tchStatus tch_gpio_handle_registerIoEvent(tch_gpio_handle_prototype* self,const tch_gpio_evCfg* cfg,const tch_IoEventCalback_t callback);
static tchStatus tch_gpio_handle_unregisterIoEvent(tch_gpio_handle_prototype* self);
static tchStatus tch_gpio_handle_configure(tch_gpio_handle_prototype* self,const tch_gpio_cfg* cfg);
static BOOL tch_gpio_handle_listen(tch_gpio_handle_prototype* self,uint32_t timeout,const tch_gpio_evCfg* cfg);

/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle);
static void tch_gpio_setIrq(tch_gpio_handle_prototype* _handle,const tch_gpio_evCfg* cfg);
static void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt);

__attribute__((section(".data"))) static tch_gpio_manager GPIO_StaticManager = {
		{
				tch_gpio_allocIo,
				tch_gpio_initCfg,
				tch_gpio_initEvCfg,
				tch_gpio_getPortCount,
				tch_gpio_getPincount,
				tch_gpio_getPinAvailable,
				tch_gpio_freeIo
		},
		MFEATURE_GPIO,
		MFEATURE_PINCOUNT_pPort
};

const tch_lld_gpio* tch_gpio_instance = (tch_lld_gpio*) &GPIO_StaticManager;


tch_gpio_handle* tch_gpio_allocIo(const gpIo_x port,uint8_t pin,const tch_gpio_cfg* cfg,tch_pwr_def pcfg){
	tch_gpio_descriptor* gpio = &GPIO_HWs[port];
	uint32_t pMsk = (1 << pin);
	if(!gpio->_clkenr){                     /// given GPIO port is not supported in this H/W
		return NULL;
	}
	if(gpio->io_ocpstate & pMsk){           /// if there is pin which is occupied by another instance
		return NULL;                        /// return null
	}

	tch_gpio_handle_prototype* instance = ((tch*)Sys)->Mem->alloc(sizeof(tch_gpio_handle_prototype));
	if(!instance)
		return NULL;
	tch_gpio_initGpioHandle(instance);
	instance->pMsk = pMsk;
	instance->idx = port;
	instance->pin = pin;

	gpio->io_ocpstate |= instance->pMsk;          /// set this pin as occupied
	GPIO_TypeDef* ioctrl_regs = (GPIO_TypeDef*) gpio->_hw;


	*gpio->_clkenr |= gpio->clkmsk;              /// enable gpio clock

	if(pcfg == ActOnSleep){                      /// if active-on-sleep is enabled
		*gpio->_lpclkenr |= gpio->lpclkmsk;      /// set lp clk bit
	}else{
		*gpio->_lpclkenr &= ~gpio->lpclkmsk;     /// otherwise clear
	}

	tch_gpio_handle_configure(instance,cfg);
	return (tch_gpio_handle*) instance;
}

void tch_gpio_initCfg(tch_gpio_cfg* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_Mode_IN;
	cfg->OSpeed = GPIO_OSpeed_2M;
	cfg->Otype = GPIO_Otype_OD;
	cfg->PuPd = GPIO_PuPd_Float;
}

void tch_gpio_initEvCfg(tch_gpio_evCfg* evcfg){
	evcfg->edge = Rise;
	evcfg->type = Interrupt;
}


uint16_t tch_gpio_getPortCount(){
	return GPIO_StaticManager.port_cnt;
}

uint16_t tch_gpio_getPincount(const gpIo_x port){
	return GPIO_StaticManager.io_cnt;
}

uint32_t tch_gpio_getPinAvailable(const gpIo_x port){
	return GPIO_HWs[port].io_ocpstate;
}

void tch_gpio_freeIo(tch_gpio_handle* IoHandle){
	tch_gpio_handle_prototype* _handle = (tch_gpio_handle_prototype*) IoHandle;
	MTX(Sys)->destroy(_handle->mtx);
}



void tch_gpio_handle_out(tch_gpio_handle_prototype* _handle,tch_bState nstate){
	if(nstate){
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR |= _handle->pMsk;
	}else{
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR &= ~_handle->pMsk;
	}
}

tch_bState tch_gpio_handle_in(tch_gpio_handle_prototype* _handle){
	return ((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->IDR & _handle->pMsk ? bSet : bClear;
}

/**
 */
tchStatus tch_gpio_handle_registerIoEvent(tch_gpio_handle_prototype* _handle,const tch_gpio_evCfg* cfg,const tch_IoEventCalback_t callback){
	tch_ioInterrupt_descriptor* ioIrqOjb = &IoInterrupt_HWs[_handle->pin];
	tchStatus result = osOK;

	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	if(_handle->cb)
		return osErrorResource;
	result = MTX(Sys)->lock(_handle->mtx,osWaitForever);
	_handle->cb = callback;
	ioIrqOjb->io_occp = _handle;
	tch_gpio_setIrq(_handle,cfg);
	result = MTX(Sys)->unlock(_handle->mtx);

	return result;
}

tchStatus tch_gpio_handle_unregisterIoEvent(tch_gpio_handle_prototype* _handle){
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[_handle->pin];
	uint32_t pMsk = (1 << _handle->pin);

	if(ioIrqObj->io_occp != _handle)
		return osErrorResource;
	NVIC_DisableIRQ(ioIrqObj->irq);
	SYSCFG->EXTICR[_handle->pin >> 2] = 0;
	EXTI->EMR &= ~pMsk;
	EXTI->IMR &= ~pMsk;
	EXTI->FTSR &= ~pMsk;
	EXTI->RTSR &= ~pMsk;
	_handle->cb = NULL;
	ioIrqObj->io_occp = NULL;

	return osOK;

}

tchStatus tch_gpio_handle_configure(tch_gpio_handle_prototype* _handle,const tch_gpio_cfg* cfg){
	GPIO_TypeDef* ioctrl_regs = (GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw;
	switch(cfg->Mode){
	case Out:
		ioctrl_regs->MODER |= (GPIO_Mode_OUT << (_handle->pin << 1));
		if(cfg->Otype == OpenDrain){
			ioctrl_regs->OTYPER |= (1 << _handle->pin);
		}else{
			ioctrl_regs->OTYPER &= ~(0 << _handle->pin);
		}
		switch(cfg->OSpeed){
		case IoSpeedLow:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_2M << (_handle->pin << 1));
			break;
		case IoSpeedMid:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_25M << (_handle->pin << 1));
			break;
		case IoSpeedHigh:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_50M << (_handle->pin << 1));
			break;
		case IoSpeedVeryHigh:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_100M << (_handle->pin << 1));
			break;
		}
		break;
	case In:
		ioctrl_regs->MODER |= (GPIO_Mode_IN << (_handle->pin << 1));
		switch(cfg->PuPd){
		case Pullup:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_PU << (_handle->pin << 1));
			break;
		case NoPull:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_Float << (_handle->pin << 1));
			break;
		case Pulldown:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_PD << (_handle->pin << 1));
			break;
		}
		break;
	case Analog:
		ioctrl_regs->MODER |= (GPIO_Mode_AN << (_handle->pin << 1));
		break;
	case Function:
		ioctrl_regs->MODER |= (GPIO_Mode_AF << (_handle->pin << 1));
	}
	return osOK;
}

BOOL tch_gpio_handle_listen(tch_gpio_handle_prototype* _handle,uint32_t timeout,const tch_gpio_evCfg* cfg){
	tch_ioInterrupt_descriptor* ioIrqObj = &IoInterrupt_HWs[_handle->pin];
	if(__get_IPSR())
		return FALSE;
	if(!ioIrqObj->io_occp){
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
		if(MTX(Sys)->lock(_handle->mtx,timeout) != osOK)
			return FALSE;
		ioIrqObj->io_occp = _handle;
		tch_gpio_setIrq(_handle,cfg);
		MTX(Sys)->unlock(_handle->mtx);
	}
	return tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&ioIrqObj->wq,timeout) == osOK ? TRUE : FALSE;
}


void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle){
	handle->_pix.in = tch_gpio_handle_in;
	handle->_pix.out = tch_gpio_handle_out;
	handle->_pix.listen = tch_gpio_handle_listen;
	handle->_pix.registerIoEvent = tch_gpio_handle_registerIoEvent;
	handle->_pix.unregisterIoEvent = tch_gpio_handle_unregisterIoEvent;
	handle->_pix.configure = tch_gpio_handle_configure;
	handle->pMsk = 0;

	tch_mtx_ix* Mtx = (tch_mtx_ix*)(((tch*) Sys)->Mtx);
	handle->mtx = Mtx->create(&handle->mtx_hdr);
	handle->cb = NULL;
	tch_genericQue_Init(&handle->wq);
}


void tch_gpio_setIrq(tch_gpio_handle_prototype* _handle,const tch_gpio_evCfg* cfg){
	tch_ioInterrupt_descriptor* ioIntObj = &IoInterrupt_HWs[_handle->pin];
	switch(cfg->edge){
	case Fall:
		EXTI->FTSR |= _handle->pMsk;
		break;
	case Rise:
		EXTI->RTSR |= _handle->pMsk;
		break;
	case Both:
		EXTI->FTSR |= _handle->pMsk;
		EXTI->RTSR |= _handle->pMsk;
		break;
	}

	switch(cfg->type){
	case Event:
		EXTI->EMR |= _handle->pMsk;
		break;
	case Interrupt:
		EXTI->IMR |= _handle->pMsk;
		break;
	}
	NVIC_SetPriority(ioIntObj->irq,HANDLER_NORMAL_PRIOR);
	NVIC_EnableIRQ(ioIntObj->irq);
	SYSCFG->EXTICR[_handle->pin >> 2] |= _handle->idx << ((_handle->pin % 4) *4);
}

void tch_gpio_handleIrq(uint8_t base_idx,uint8_t group_cnt){
	uint32_t ext_pr = (EXTI->PR >> base_idx);
	tch_ioInterrupt_descriptor* ioIntObj = NULL;
	tch_gpio_handle_prototype* _handle = NULL;
	uint8_t pos = 0;
	uint32_t pMsk = 1;
	while(ext_pr){
		if(ext_pr & 1){
			ioIntObj = &IoInterrupt_HWs[base_idx + pos++];
			if(!ioIntObj->io_occp)
				return;
			_handle = (tch_gpio_handle_prototype*)ioIntObj->io_occp;
			if(_handle->cb)
				_handle->cb((tch_gpio_handle*)_handle,_handle->pin);
			EXTI->PR |= pMsk;
			if(ioIntObj->wq.entry)
				tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&ioIntObj->wq,0);
		}
		ext_pr >>= 1;
		pMsk <<= 1;
		if(!(pos < group_cnt))
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
	tch_gpio_handleIrq(2,0);
}

void EXTI3_IRQHandler(void){
	tch_gpio_handleIrq(3,0);
}

void EXTI4_IRQHandler(void){
	tch_gpio_handleIrq(4,0);
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

