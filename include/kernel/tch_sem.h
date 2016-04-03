/*
 * tch_sem.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_SEM_H_
#define TCH_SEM_H_


#include "tch_kobj.h"
#include "tch_types.h"

#if defined(__cplusplus)
extern "C"{
#endif

/***
 *  semaphore  types
 */
typedef struct tch_semCb {
	tch_kobj          	__obj;
	uint32_t        	state;
	uint32_t     	    count;
	dlistEntry_t        wq;
} tch_semCb;


extern tch_semaphore_api_t Semaphore_IX;
extern tchStatus tch_semInit(tch_semCb* scb,uint32_t count);
extern tchStatus tch_semDeinit(tch_semCb* scb);


#if defined(__cplusplus)
}
#endif

#endif /* TCH_SEM_H_ */
