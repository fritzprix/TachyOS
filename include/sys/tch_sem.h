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



/***
 *  semaphore  types
 */
typedef void* tch_sem_id;
typedef struct _tch_sem_t {
	uint8_t           key;
	uint32_t          count;
	tch_lnode_t       wq;
} tch_semDef;

struct _tch_semaph_ix_t {
	tch_sem_id (*create)(tch_semDef* sem,uint32_t count);
	tchStatus (*lock)(tch_sem_id sid,uint32_t timeout);
	tchStatus (*unlock)(tch_sem_id sid);
	tchStatus (*destroy)(tch_sem_id sid);
};


#endif /* TCH_SEM_H_ */
