/*
 * mem.c
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */


#include "mem.h"
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
	return TRUE;
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

char* tch_itoa(int num,char* rb,int base){
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
