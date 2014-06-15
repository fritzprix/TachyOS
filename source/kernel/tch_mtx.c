/*
 * tch_mtx.c
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */



#include "tch.h"



static void tch_mtx_create(tch_mtx_id* mtx);
static osStatus tch_mtx_lock(tch_mtx_id* mtx,uint32_t timeout);
static osStatus tch_mtx_unlock(tch_mtx_id* mtx);
static osStatus tch_mtx_destroy(tch_mtx_id* mtx);



