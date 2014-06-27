/*
 * mem.h
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */

#ifndef TCH_LIB_H_
#define TCH_LIB_H_
#include "../tch.h"


int tch_memcpy(uint8_t* dest,const uint8_t* src,int l);
int tch_memcmp(const uint8_t* dest,const uint8_t* src,int size);
BOOL tch_memset(uint8_t* bp,uint32_t size,const uint8_t v);
int tch_strconcat(char* dest,const char* str1,const char* str2);
int tch_strlen(char* str);
int tch_strSplit(char* dest,char* src,char delmit,char** splt_result);
BOOL tch_strcmp(const char* str1,const char* str2);
void tch_strcpy(char* dest,char* src);
long tch_atoi(const char* cstr);
char* tch_itoa(long num,char* rb,int base);


#endif /* MEM_H_ */
