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
extern void* tch_memAlloc(tch_memId mh,size_t size);
extern void tch_memFree(tch_memId mh,void* p);
extern uint32_t tch_memAvail(tch_memId mh);
extern tchStatus tch_memForceRelease(tch_memId mh,tch_lnode_t* alloc_list);


#if defined(__cplusplus)
}
#endif
#endif /* TCH_MEM_H_ */
