/*
 * mem.h
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */

#ifndef MEM_H_
#define MEM_H_
#include "../core/port/tch_stdtypes.h"


int tch_memcpy(uint8_t* dest,const uint8_t* src,int l);
int tch_memcmp(const uint8_t* dest,const uint8_t* src,int size);
BOOL tch_memset(uint8_t* bp,uint32_t size,const uint8_t v)__attribute__((always_inline));
int tch_strconcat(char* dest,const char* str1,const char* str2);
char* tch_itoa(int num,char* rb,int base);


#endif /* MEM_H_ */
