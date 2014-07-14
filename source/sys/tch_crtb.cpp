/*
 * tch_crtb.cpp
 *
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 14.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_lib.h"

void* operator new(size_t size) throw() { return ((tch*)Sys)->Mem->alloc(size);}
void operator delete(void* p){((tch*)Sys)->Mem->free(p);}

extern "C" int __aeabi_atexit(void *object,void (*)(void*),void *dso_handle){
	return 0;
}

extern "C" void* memset(void* ptr,int value,size_t num){
	while(num--){
		*((uint8_t*)ptr + num) = value;
	}
	return ptr;
}
