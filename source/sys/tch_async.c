/*
 * tch_async.c
 *
 *  Created on: 2014. 8. 8.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_async.h"



#define TCH_ASYNC_PRIOR_VERYHIGH      ((uint8_t) 5)
#define TCH_ASYNC_PRIOR_HIGH          ((uint8_t) 4)
#define TCH_ASYNC_PRIOR_NORMAL        ((uint8_t) 3)
#define TCH_ASYNC_PRIOR_LOW           ((uint8_t) 2)
#define TCH_ASYNC_PRIOR_VERYLOW       ((uint8_t) 1)

static tch_async_id tch_async_create(tch_async_routine fn,void* arg,uint8_t prior);
static tchStatus tch_async_start(tch_async_id async);
static tchStatus tch_async_blockedstart(tch_async_id async,uint32_t timeout);
static void tch_async_notify(tch_async_id id,tchStatus res);
static void tch_async_destroy(tch_async_id async);




__attribute__((section(".data"))) static tch_async_ix ASYNC_StaticInstance = {
		{
				TCH_ASYNC_PRIOR_VERYHIGH,
				TCH_ASYNC_PRIOR_HIGH,
				TCH_ASYNC_PRIOR_NORMAL,
				TCH_ASYNC_PRIOR_LOW,
				TCH_ASYNC_PRIOR_VERYLOW
		},
		tch_async_create,
		tch_async_start,
		tch_async_blockedstart,
		tch_async_notify,
		tch_async_destroy
};

extern const tch_async_ix* Async = &ASYNC_StaticInstance;




static tch_async_id tch_async_create(tch_async_routine fn,void* arg,uint8_t prior){
	tch_async_cb* cb = (tch_async_cb*) malloc(sizeof(tch_async_cb));
	cb->arg = arg;
	cb->fn = fn;
	tch_listInit((tch_lnode_t*)&cb->wq);
	tch_listInit(&cb->ln);

	return (tch_async_id)cb;
}

static tchStatus tch_async_start(tch_async_id async){
	if(__get_IPSR()){
		return tch_port_enterSvFromIsr(SV_ASYNC_START,async,0);
	}else{
		return tch_port_enterSvFromUsr(SV_ASYNC_START,async,0);
	}
}

static tchStatus tch_async_blockedstart(tch_async_id async,uint32_t timeout){
	if(__get_IPSR()){
		return tch_port_enterSvFromIsr(SV_ASYNC_BLSTART,async,timeout);
	}else{
		return tch_port_enterSvFromUsr(SV_ASYNC_BLSTART,async,timeout);
	}
}

static void tch_async_notify(tch_async_id id,tchStatus res){
	if(__get_IPSR()){
		tch_port_enterSvFromIsr(SV_ASYNC_NOTIFY,id,res);
	}else{
		tch_port_enterSvFromUsr(SV_ASYNC_NOTIFY,id,res);
	}
}


static void tch_async_destroy(tch_async_id async){

}

LIST_CMP_FN(tch_async_comp){
	return ((tch_async_cb*)prior)->prior > ((tch_async_cb*)post)->prior;
}




