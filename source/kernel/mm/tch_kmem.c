/*
 * tch_kmem.c
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */



#include "tch_kernel.h"
#include "tch_mm.h"

#define KERNEL_STACK_OVFCHECK_MAGIC			((uint32_t)0xFF00FF0)


/**
 * when region is freed first page of region has header for region information
 */

static uint8_t __kstack __kernel_stack[CONFIG_KERNEL_STACKSIZE];


uint32_t* tch_kernelMemInit(){

}



