/*
 * mem.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */

#ifndef TCH_LIB_H_
#define TCH_LIB_H_
#include "tch.h"


int tch_memcpy(uint8_t* dest,const uint8_t* src,int l);
int tch_memcmp(const uint8_t* dest,const uint8_t* src,int size);
BOOL tch_memset(uint8_t* bp,const uint8_t v,uint32_t size);
int tch_strconcat(char* dest,const char* str1,const char* str2);
int tch_strlen(char* str);
int tch_strSplit(char* dest,char* src,char delmit,char** splt_result);
BOOL tch_strcmp(const char* str1,const char* str2);
void tch_strcpy(char* dest,char* src);
long tch_atoi(const char* cstr);
char* tch_itoa(long num,char* rb,int base);


#endif /* MEM_H_ */
