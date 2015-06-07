/*
 * tch_syscall.h
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */

#ifndef TCH_SYSCALL_H_
#define TCH_SYSCALL_H_
#include "tch_ktypes.h"

typedef tchStatus (*SYSCALL_1A_T)(uword_t arg);


void __SYSCALL_DUMMY_FUNC() { }
void (*const __SYSCALL_ENTRY)() __attribute__((section(".sysc.entry"))) = __SYSCALL_DUMMY_FUNC;



#define SYSCALL_1(sysc_id,__fname,__pname,__ptype)								\
extern tchStatus __fname(__ptype __pname);					\
const  SYSCALL_1A_T __fname##_syscall_vec __attribute__((section(".sysc.table"))) = __fname;\
const  uint8_t sysc_id = (size_t)__fname##_syscall_vec - (size_t)__SYSCALL_ENTRY




#endif /* TCH_SYSCALL_H_ */
