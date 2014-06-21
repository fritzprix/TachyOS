/*
 * mem.c
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */


#include "tch_lib.h"
int tch_memcpy(uint8_t* dest,const uint8_t* src,int l){
	int resLen = l;
	while(l--){
		*dest++ = *src++;
	}
	return resLen;
}

int tch_memcmp(const uint8_t* dest,const uint8_t* src,int size){
	while(size--){
		if(*dest++ != *src++)
			return 0;
	}
	return 1;
}


BOOL tch_memset(uint8_t* dest,uint32_t size,const uint8_t v){
	while(size--){
		*dest++ = v;
	}
	return true;
}


int tch_strconcat(char* dest,const char* str1,const char* str2){
	char* tdest = dest;
	while(*str1 != '\0'){
		*tdest++ = *str1++;
	}
	while(*str2 != '\0'){
		*tdest++ = *str2++;
	}
	*tdest = '\0';
	return tdest - dest;
}

int tch_strSplit(char* dest,char* src,char delmit,char** splt_list){
	char* tstr = src;
	*splt_list++= src;
	int cnt = 1;
	while(*tstr != '\0'){
		*dest++ = *tstr;
		if(*tstr++ == delmit){
			*(dest - 1) = '\0';
			cnt++;
			*splt_list++ = dest;
		}
	}
	return cnt;
}

int tch_strlen(char* str){
	int cnt = 0;
	while(*str++ != '\0')
		cnt++;
	return cnt;
}

BOOL tch_strcmp(const char* str1,const char* str2){
	char* tstr1 = (char*) str1;
	char* tstr2 = (char*) str2;
	while(*tstr1 != '\0'){
		if(*tstr1++ != *tstr2++){
			return false;
		}
	}
	return true;
}

void tch_strcpy(char* dest,char* src){
	while(*src != '\0'){
		*dest++ = *src++;
	}
}

long tch_atoi(const char* cstr){
	int len = tch_strlen((char*)cstr);
	uint16_t idx = len;
	int64_t base = 1;
	int64_t res = 0;
	while(idx--){
		res += (*(cstr + idx) - 48) * base;
		base *= 10;
	}
	return res;
}

char* tch_itoa(long num,char* rb,int base){
	int mlbm = base;
	char* trb = rb;
	while((num / mlbm) != 0){
		mlbm *= base;
	}
	mlbm /= base;
	while(mlbm != 0){
		*trb = (num / mlbm) + 48;
		num -= (*trb++ - 48) * mlbm;
		mlbm /= base;
	}
	*trb = '\0';
	return rb;
}
