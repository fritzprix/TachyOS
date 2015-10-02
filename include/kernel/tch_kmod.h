/*
 * tch_kmod.h
 *
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2015. 10. 1.
 *      Author: innocentevil
 */

#ifndef TCH_KMOD_H_
#define TCH_KMOD_H_

#include "tch_types.h"
#include "kernel/util/cdsl_rbtree.h"
#include "kernel/tch_kernel.h"

#if defined(__cplusplus)
extern "C" {
#endif


/**
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  +                                        kernel module interface                                                          +
 *  +                                                                                                                         +
 *  +                                                                                                                         +
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

typedef int (*__mod_init_t) (void);
typedef void (*__mod_exit_t) (void);

typedef struct module_header {
	rb_treeNode_t 			rbn;
	void*					uaccess_ix;
} module_header_t;

#define MODULE_INIT(init) 	 __attribute__((section(".initv_array"))) __mod_init_t ___##init_VECTOR = init;
#define MODULE_EXIT(exit)    __attribute__((section(".exitv_array"))) __mod_exit_t ___##exit_VECTOR = exit;

extern BOOL tch_registerInterface(int key, __UACESS void* interface);
extern BOOL tch_unregisterInterface(int key);


#if defined(__cplusplus)
}
#endif



#endif /* TCH_KMOD_H_ */
