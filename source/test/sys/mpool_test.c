/*
 * mpool_test.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 5.
 *      Author: innocentevil
 */


#include "mpool_test.h"
#include "tch.h"


typedef struct person {
	uint32_t age;
	uint32_t sex;
} person;


tch_mpoolDef(person_mp,10,person);
tch_mpool_id person_mpool;
person* ps[10];

tchStatus do_mpoolBaseTest(tch* api){
	person_mpool = api->Mempool->create(tch_access_mpool(person_mp));
	uint32_t i = 0;
	/***
	 *  allocation / free repeat test
	 */
	for(i = 0;i < 100;i++){
		person* p = api->Mempool->calloc(person_mpool);
		api->Mempool->free(person_mpool,p);
	}

	/****
	 *  full allocation check
	 */
	for(i = 0;i < 10;i++){
		ps[i] = api->Mempool->calloc(person_mpool);
		ps[i]->age = i;
	}

	for(i = 0;i < 10;i++){
		if(ps[i]->age != i)
			return osErrorOS;
	}

	person* pn = api->Mempool->calloc(person_mpool);
	if(pn)                      ///< should be null
		return osErrorOS;
	if(osOK == api->Mempool->free(person_mpool,NULL))
		return osErrorOS;

	return osOK;
}
