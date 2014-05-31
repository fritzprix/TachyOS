/*
 * mem.c
 *
 *  Created on: 2014. 2. 22.
 *      Author: innocentevil
 */


#include "mem.h"
int fmo_memcpy(uint8_t* dest,const uint8_t* src,int l){
	int resLen = l;
	while(l--){
		*dest++ = *src++;
	}
	return resLen;
}

int fmo_memcmp(const uint8_t* dest,const uint8_t* src,int size){
	while(size--){
		if(*dest++ != *src++)
			return 0;
	}
	return 1;
}


int fmo_memset(uint8_t* dest,uint32_t size,const uint8_t v){
	while(size--){
		*dest++ = v;
	}
	return SUCCESS;
}

