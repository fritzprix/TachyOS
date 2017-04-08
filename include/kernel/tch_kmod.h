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

#ifndef TCH_KMODULE_H_
#define TCH_KMODULE_H_

#include "tch_types.h"
#include "cdsl_rbtree.h"
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



#define set_module_type(map,type)			map->_map[type / 64] |= (1 << (type % 64))
#define clr_module_type(map,type)			map->_map[type / 64] &= ~(1 << (type % 64))

typedef int (*initv_t) (void);
typedef void (*exitv_t) (void);


#define MODULE_INIT(init) 	 __attribute__((section(".sinitv"))) const initv_t  ___##init##_vector = init
#define MODULE_EXIT(exit)    __attribute__((section(".sexitv"))) const exitv_t  ___##exit##_vector = exit


extern tch_module_api_t Module_IX;


extern BOOL tch_kmod_init(void);
extern void tch_kmod_exit(void);
extern BOOL tch_kmod_register(int type,int owner, __UACESS void* interface,BOOL ispriv);
extern BOOL tch_kmod_unregister(int type,int owner);
extern BOOL tch_kmod_chkdep(const module_map_t* map);
extern void* tch_kmod_request(int type);




#if defined(__cplusplus)
}
#endif



#endif /* TCH_KMODULE_H_ */
