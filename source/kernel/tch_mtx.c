/*
 * tch_mtx.c
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */



#include "kernel/tch_kernel.h"



static tch_mtx_id tch_mtx_create(tch_mtx* mtx);
static osStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout);
static osStatus tch_mtx_unlock(tch_mtx_id mtx);
static osStatus tch_mtx_destroy(tch_mtx_id mtx);









__attribute__((section(".data"))) static tch_mtx_ix MTX_Instance = {
		tch_mtx_create,
		tch_mtx_lock,
		tch_mtx_unlock,
		tch_mtx_destroy,
};



tch_mtx_id tch_mtx_create(tch_mtx* mtx){
	mtx->key = 0;
	tch_genericQue_Init(&mtx->que);
	return (tch_mtx_id) mtx;
}

osStatus tch_mtx_lock(tch_mtx_id mtx,uint32_t timeout){

}

osStatus tch_mtx_unlock(tch_mtx_id mtx){

}

osStatus tch_mtx_destroy(tch_mtx_id mtx){

}
