/*
 * tch_condv.h
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_CONDV_H_
#define TCH_CONDV_H_


/***
 *  condition variable types
 */
typedef void* tch_condv_id;
typedef struct _tch_condv_t tch_condv;

struct _tch_condvar_ix_t {
	tch_condv_id (*create)(tch_condv* condv);
	BOOL (*wait)(tch_condv* condv,uint32_t timeout);
	osStatus (*wake)(tch_condv* condv);
	osStatus (*wakeAll)(tch_condv* condv);
	osStatus (*destroy)(tch_condv* condv);
};



#endif /* TCH_CONDV_H_ */
