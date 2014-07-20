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
	uint32_t                      pin;
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
static uint16_t tch_gpio_getPortCount();
static uint16_t tch_gpio_getPincount(const gpIo_x port);
static uint32_t tch_gpio_getPinAvailable(const gpIo_x port);
static void tch_gpio_freeIo(tch_gpio_handle* IoHandle);


/**********************************************************************************************/
/************************************  gpio handle public function ****************************/
/**********************************************************************************************/
static void tch_gpio_handle_out(tch_gpio_handle* self,tch_bState nstate);
static tch_bState tch_gpio_handle_in(tch_gpio_handle* self);
static void tch_gpio_handle_registerIoEvent(tch_gpio_handle* self,tch_IoEventCalback_t callback);
static void tch_gpio_handle_unregisterIoEvent(tch_gpio_handle* self);
static BOOL tch_gpio_handle_listen(tch_gpio_handle* self,uint32_t timeout);

/**********************************************************************************************/
/************************************     private fuctnions   *********************************/
/**********************************************************************************************/
static void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle);

__attribute__((section(".data"))) static tch_gpio_manager GPIO_StaticManager = {
		{
				tch_gpio_allocIo,
				tch_gpio_initCfg,
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
	if(!gpio->_clkenr){                     /// given GPIO port is not supported in this H/W
		return NULL;
	}
	uint32_t pMsk = (1 << pin);
	if(gpio->io_ocpstate & pMsk){           /// if there is pin which is occupied by another instance
		return NULL;                        /// return null
	}

	tch_gpio_handle_prototype* instance = ((tch*)Sys)->Mem->alloc(sizeof(tch_gpio_handle_prototype));
	if(!instance)
		return NULL;
	tch_gpio_initGpioHandle(instance);
	gpio->io_ocpstate |= pMsk;               /// set this pin as occupied
	GPIO_TypeDef* ioctrl_regs = (GPIO_TypeDef*) gpio->_hw;


	*gpio->_clkenr |= gpio->clkmsk;          /// enable gpio clock

	if(pcfg == ActOnSleep){                  /// if active-on-sleep is enabled
		*gpio->_lpclkenr |= gpio->lpclkmsk;  /// set lp clk bit
	}else{
		*gpio->_lpclkenr &= ~gpio->lpclkmsk; /// otherwise clear
	}

	switch(cfg->Mode){
	case Out:
		ioctrl_regs->MODER |= (GPIO_Mode_OUT << (pin << 1));
		if(cfg->Otype == OpenDrain){
			ioctrl_regs->OTYPER |= (1 << pin);
		}else{
			ioctrl_regs->OTYPER &= ~(0 << pin);
		}
		switch(cfg->OSpeed){
		case IoSpeedLow:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_2M << (pin << 1));
			break;
		case IoSpeedMid:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_25M << (pin << 1));
			break;
		case IoSpeedHigh:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_50M << (pin << 1));
			break;
		case IoSpeedVeryHigh:
			ioctrl_regs->OSPEEDR |= (GPIO_OSpeed_100M << (pin << 1));
			break;
		}
		break;
	case In:
		ioctrl_regs->MODER |= (GPIO_Mode_IN << (pin << 1));
		switch(cfg->PuPd){
		case Pullup:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_PU << (pin << 1));
			break;
		case NoPull:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_Float << (pin << 1));
			break;
		case Pulldown:
			ioctrl_regs->PUPDR |= (GPIO_PuPd_PD << (pin << 1));
			break;
		}
		break;
	case Analog:
		ioctrl_regs->MODER |= (GPIO_Mode_AN << (pin << 1));
		break;
	case Function:
		ioctrl_regs->MODER |= (GPIO_Mode_AF << (pin << 1));
	}
	instance->idx = port;
	instance->pin = pMsk;

	return (tch_gpio_handle*) instance;
}

void tch_gpio_initCfg(tch_gpio_cfg* cfg){

	cfg->Af = 0;
	cfg->Mode = GPIO_Mode_IN;
	cfg->OSpeed = GPIO_OSpeed_2M;
	cfg->Otype = GPIO_Otype_OD;
	cfg->PuPd = GPIO_PuPd_Float;
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
	tch_mtx_ix* Mtx = ((tch*)Sys)->Mtx;
	Mtx->destroy(&_handle->mtx);                                      /// destroy Mtx
}



void tch_gpio_handle_out(tch_gpio_handle* self,tch_bState nstate){
	tch_gpio_handle_prototype* _handle = (tch_gpio_handle_prototype*) self;
	if(nstate){
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR |= _handle->pin;
	}else{
		((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->ODR &= ~_handle->pin;
	}
}

tch_bState tch_gpio_handle_in(tch_gpio_handle* self){
	tch_gpio_handle_prototype* _handle = (tch_gpio_handle_prototype*) self;
	return ((GPIO_TypeDef*)GPIO_HWs[_handle->idx]._hw)->IDR & _handle->pin ? bSet : bClear;
}


void tch_gpio_handle_registerIoEvent(tch_gpio_handle* self,tch_IoEventCalback_t callback){

}

void tch_gpio_handle_unregisterIoEvent(tch_gpio_handle* self){

}

BOOL tch_gpio_handle_listen(tch_gpio_handle* self,uint32_t timeout){

}


void tch_gpio_initGpioHandle(tch_gpio_handle_prototype* handle){
	handle->_pix.in = tch_gpio_handle_in;
	handle->_pix.out = tch_gpio_handle_out;
	handle->_pix.listen = tch_gpio_handle_listen;
	handle->_pix.registerIoEvent = tch_gpio_handle_registerIoEvent;
	handle->_pix.unregisterIoEvent = tch_gpio_handle_unregisterIoEvent;

	tch_mtx_ix* Mtx = ((tch*) Sys)->Mtx;
	handle->mtx = Mtx->create(&handle->mtx_hdr);
	tch_genericQue_Init(&handle->wq);
}
