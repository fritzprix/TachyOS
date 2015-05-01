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
#include "tch_list.h"

#if defined(__cplusplus)
extern "C"{
#endif



/**
 * base heap implementation of tachyos
 */
extern tch_memId tch_memInit(void* mem,uint32_t sz,BOOL isMultiThreaded);

/**\brief Memory Allocation function
 * \param[in] mh memory allocator id
 * \param[in] size allocation size
 * \param[in] optional allocation list which can be NULL
 */
extern void* tch_memAlloc(tch_memId mh,size_t size,tch_lnode* alc_le);

/**\brief Memory Free Function
 * \param[in] mh memory allocator id
 * \param[in] p pointer to memory chunk to be freed
 */
extern tchStatus tch_memFree(tch_memId mh,void* p,tch_lnode* alc_le);

/**\brief Function for freeing all allocated memory chunk
 * \param[in] mh memory allocator id
 * \param[in] entry of allocation list which holds allocated chunk
 */
extern tchStatus tch_memFreeAll(tch_memId mh,tch_lnode* alc_le,BOOL exec_destr);

/**\brief get size of avaiable memory from memory allocator
 * \param[in] mh memory allocator id
 */
extern uint32_t tch_memAvail(tch_memId mh);





#if defined(__cplusplus)
}
#endif
#endif /* TCH_MEM_H_ */
