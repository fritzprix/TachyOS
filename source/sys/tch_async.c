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

#define TCH_ASYNC_STATUS_INIT         ((uint8_t) 1)
#define TCH_ASYNC_STATUS_ONQUEUE_BLK  ((uint8_t) 2)
#define TCH_ASYNC_STATUS_ONQUEUE_NBLK ((uint8_t) 4)
#define TCH_ASYNC_STATUS_NOTIFIED     ((uint8_t) 8)

#define TCH_ASYNC_STATUS_ONQUEUE      (TCH_ASYNC_STATUS_ONQUEUE_BLK | TCH_ASYNC_STATUS_ONQUEUE_NBLK)

#define SET_STATUS(id,stat)           {((tch_async_cb*)id)->status = stat;}
#define GET_STATUS(id)                 (((tch_async_cb*)id)->status)

static tch_async_id tch_async_create(tch_async_routine fn,void* arg,uint8_t prior);
static tchStatus tch_async_start(tch_async_id id);
static tchStatus tch_async_blockedstart(tch_async_id id,uint32_t timeout);
static void tch_async_notify(tch_async_id id,tchStatus res);
static void tch_async_destroy(tch_async_id id);




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

const tch_async_ix* Async = &ASYNC_StaticInstance;




static tch_async_id tch_async_create(tch_async_routine fn,void* arg,uint8_t prior){
	tch_async_cb* cb = (tch_async_cb*) malloc(sizeof(tch_async_cb));
	cb->arg = arg;
	cb->fn = fn;
	cb->own = Thread->self();
	tch_listInit((tch_lnode_t*)&cb->wq);
	tch_listInit(&cb->ln);

	SET_STATUS(cb,TCH_ASYNC_STATUS_INIT);
	return (tch_async_id)cb;
}

static tchStatus tch_async_start(tch_async_id id){
	if(((tch_async_cb*)id)->own != Thread->self())
		return osErrorOS;
	SET_STATUS(id,TCH_ASYNC_STATUS_ONQUEUE_BLK);
	if(__get_IPSR()){
		return osErrorISR;
	}else{
		tch_port_enterSvFromUsr(SV_ASYNC_START,id,0);
		return osOK;
	}
	return osErrorOS;
}

static tchStatus tch_async_blockedstart(tch_async_id id,uint32_t timeout){
	tchStatus result = osOK;
	if(((tch_async_cb*)id)->own != Thread->self())
		return osErrorOS;
	SET_STATUS(id,TCH_ASYNC_STATUS_ONQUEUE_BLK);
	if(__get_IPSR()){
		return osErrorISR;
	}else{
		 result = tch_port_enterSvFromUsr(SV_ASYNC_BLSTART,id,timeout);
		 SET_STATUS(id,TCH_ASYNC_STATUS_NOTIFIED);
		 return result;
	}
}

static void tch_async_notify(tch_async_id id,tchStatus res){
	switch(GET_STATUS(id)){
	case TCH_ASYNC_STATUS_ONQUEUE_BLK:
		if(__get_IPSR()){
			tch_port_enterSvFromIsr(SV_ASYNC_NOTIFY,id,res);
		}else{
			tch_port_enterSvFromUsr(SV_ASYNC_NOTIFY,id,res);
		}
		break;
	case TCH_ASYNC_STATUS_ONQUEUE_NBLK:
		SET_STATUS(id,TCH_ASYNC_STATUS_NOTIFIED);
		break;
	default:
		// error condition : no operation when async not started notified
		break;
	}
}


static void tch_async_destroy(tch_async_id id){
	if(((tch_async_cb*)id)->own != Thread->self())
		return;
	while(GET_STATUS(id) & TCH_ASYNC_STATUS_ONQUEUE)
		Thread->sleep(0);                                          // yield wait until async is possible to be destroied
	free((void*)id);
}

LIST_CMP_FN(tch_async_comp){
	return ((tch_async_cb*)prior)->prior > ((tch_async_cb*)post)->prior;
}




