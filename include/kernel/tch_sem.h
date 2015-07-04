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

#include "tch_TypeDef.h"


#if defined(__cplusplus)
extern "C"{
#endif


/***
 *  semaphore  types
 */

//SYSCALL_1(SEMAPHORE_CREATE,semaphore_create,count,uint32_t);

extern tchStatus semaphore_create(uint32_t count);
//static  uint8_t sysc_id = (const size_t) semaphore_create_syscall_vec - (const size_t) __SYSCALL_ENTRY;


#if defined(__cplusplus)
}
#endif

#endif /* TCH_SEM_H_ */
