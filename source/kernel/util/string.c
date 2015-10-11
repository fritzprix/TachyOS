/*
 * string.c
 *
 *  Created on: 2015. 10. 3.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/util/string.h"


void memset(void* dst,int v,size_t sz){
	if(!dst)
		return;

	while(sz--) ((uint8_t*) dst)[sz] = v;
}

void memcpy(void* dst,const void* src,size_t n){
	if(!dst || !src)
		return;

	while(n--)((uint8_t*) dst)[n] = ((uint8_t*) src)[n];
}

int memcmp(const void* s1,const void* s2,size_t n){
	if(!s1 || !s2)
		return -1;
	size_t i;
	uint8_t v;
	for(i = 0;i < n; i++){
		if(((v = ((uint8_t*) s1)[i] - ((uint8_t*) s2)[i])) != 0)
			return v < 0? -1 : 1;
	}
	return 0;
}
