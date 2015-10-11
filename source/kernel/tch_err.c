/*
 * tch_err.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_err.h"



void tch_kernel_raiseError(tch_threadId who,int errno,const char* msg){			//managable error which is cuased by thread level erroneous behavior
	while(TRUE){
		__WFE();
	}
}

void tch_kernel_panic(const char* floc,int lno, const char* msg){
	while(TRUE){
		__WFE();
	}
}

void tch_kernel_handleHardFault(int fault){
	while(TRUE){
		__WFE();
	}
}
