/*
 * mmap.h
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */

#ifndef MMAP_H_
#define MMAP_H_


#include "port/tch_stdtypes.h"

#define __MAIN_STACK_SIZE        (uint32_t) (1 << 8)
#define __INIT_STACK_SIZE        (uint32_t) (1 << 10)
#define __MEM_GUARD_REGION       (uint32_t) (10)



#define _GET_SYS_STACK_BOTTOM(_ebss) (uint32_t*) (&_ebss + __MEM_GUARD_REGION)
#define _GET_SYS_STACK_TOP(_ebss) (uint32_t*) (_GET_SYS_STACK_BOTTOM(_ebss) + __MAIN_STACK_SIZE)

#define _GET_INIT_STACK_BOTTOM(_ebss) (uint32_t*) (_GET_SYS_STACK_TOP(_ebss) + __MEM_GUARD_REGION)
#define _GET_INIT_STACK_TOP(_ebss) (uint32_t*) (_GET_INIT_STACK_BOTTOM(_ebss) + __INIT_STACK_SIZE)



extern const uint32_t _ebss;

const uint32_t __mthread_stack_bottom = _GET_INIT_STACK_BOTTOM(_ebss);
const uint32_t __mthread_stack_top    = _GET_INIT_STACK_TOP(_ebss);


const uint32_t __sys_stack_bottom = _GET_SYS_STACK_BOTTOM(_ebss);
const uint32_t __sys_stack_top    = _GET_SYS_STACK_TOP(_ebss);





#endif /* MMAP_H_ */
