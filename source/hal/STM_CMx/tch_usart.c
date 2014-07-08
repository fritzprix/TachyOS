/*
 * tch_usart.c
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


#include "hal/STM_CMx/tch_hal.h"

typedef struct tch_lld_usart_prototype {
	tch_lld_usart                         pix;
}tch_lld_usart_prototype;

__attribute__((section(".data"))) static tch_lld_usart_prototype USART_StaticInstance = {

};


const tch_lld_usart* tch_usart_instance = (const tch_lld_usart*) &USART_StaticInstance;
