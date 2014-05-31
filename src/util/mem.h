/*
 * mem.h
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */

#ifndef MEM_H_
#define MEM_H_
#include "stm32f4xx.h"


int fmo_memcpy(uint8_t* dest,const uint8_t* src,int l);
int fmo_memcmp(const uint8_t* dest,const uint8_t* src,int size);
int fmo_memset(uint8_t* bp,uint32_t size,const uint8_t v)__attribute__((always_inline));


#endif /* MEM_H_ */
