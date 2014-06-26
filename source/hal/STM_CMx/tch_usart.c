/*
 * tch_usart.c
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
