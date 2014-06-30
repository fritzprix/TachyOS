/*
 * tch_sem.h
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
typedef struct _tch_sem_t tch_sem;

struct _tch_semaph_ix_t {
	tch_sem_id (*create)(tch_sem* sem);
	osStatus (*lock)(tch_sem_id sid,uint32_t timeout);
	osStatus (*unlock)(tch_sem_id sid);
	osStatus (*destroy)(tch_sem_id sid);
};


#endif /* TCH_SEM_H_ */
