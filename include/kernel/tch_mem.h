/*
 * tch_mem.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */

#ifndef TCH_MEM_H_
#define TCH_MEM_H_


#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif

extern tch_memId tch_memCreate(void* mem,uint32_t sz);
extern tchStatus tch_noop_destr(tch_uobj* obj);

/**
 * @brief This is public API for Dynamic Memory Allocation
 */

#if defined(__cplusplus)
}
#endif
#endif /* TCH_MEM_H_ */
