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


#include <stdlib.h>
#include "tch_kernel.h"

void* operator new(size_t size) throw() {
#ifdef __USE_MALLOC
	return malloc(size);
#else
	return uMem->alloc(size);
#endif
}
void operator delete(void* p){
#ifdef __USE_MALLOC
	free(p);
#else
	uMem->free(p);
#endif
}

extern "C" int __aeabi_atexit(void *object,void (*)(void*),void *dso_handle){
	return 0;
}

