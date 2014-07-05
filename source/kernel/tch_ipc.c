/*
 * tch_ipc.c
 *
 *  Created on: 2014. 7. 5.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "lib/tch_lib.h"


static tch_msgQue_id tch_msgQ_create(const tch_msgQueDef_t* que);
static osStatus tch_msgQ_put(tch_msgQue_id,uint32_t msg,uint32_t millisec);
static osEvent tch_msgQ_get(tch_msgQue_id,uint32_t millisec);


static tch_mailQue_id tch_mailQ_create(const tch_mailQueDef_t* que);
static void* tch_mailQ_alloc(tch_mailQue_id qid,uint32_t millisec);
static void* tch_mailQ_calloc(tch_mailQue_id qid,uint32_t millisec);
static osStatus tch_mailQ_put(tch_mailQue_id qid);
static osEvent tch_mailQ_get(tch_mailQue_id qid,uint32_t millisec);
static osStatus tch_mailQ_free(tch_mailQue_id qid,void* mail);



__attribute__((section(".data"))) static tch_msgq_ix MsgQStaticInstance = {
		tch_msgQ_create,
		tch_msgQ_put,
		tch_msgQ_get
};

__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		tch_mailQ_create,
		tch_mailQ_alloc,
		tch_mailQ_calloc,
		tch_mailQ_put,
		tch_mailQ_get,
		tch_mailQ_free
};

const tch_msgq_ix* MsgQ = &MsgQStaticInstance;
const tch_mailq_ix* MailQ = &MailQStaticInstance;

tch_msgQue_id tch_msgQ_create(const tch_msgQueDef_t* que){
	tch_memset(que->pool,que->item_sz * que->queue_sz,0);
	tch_msgq_instance* msgq_header = (tch_msgq_instance*) que->pool - 1;
	msgq_header->pidx = 0;
	msgq_header->gidx = 0;
	msgq_header->psize = 0;
	msgq_header->msgDef = que;
	tch_genericQue_Init(&msgq_header->msgputWq);
	tch_genericQue_Init(&msgq_header->msggetWq);
	return (tch_msgQue_id) msgq_header;
}

osStatus tch_msgQ_put(tch_msgQue_id que_id,uint32_t msg,uint32_t millisec){
	tch_msgq_instance* mq_header = (tch_msgq_instance*) que_id;
	if(tch_port_isISR()){
		if(millisec)
			return osErrorParameter;
		if(mq_header->psize >= mq_header->msgDef->queue_sz)
			return osErrorResource;
		tch_port_kernel_lock();
		mq_header->psize++;
		*((uint32_t*)mq_header->msgDef->pool + mq_header->pidx++) = msg;
		if(mq_header->pidx >= mq_header->msgDef->queue_sz)
			mq_header->pidx = 0;
		tch_port_kernel_unlock();
		return osOK;
	}else{
		if(mq_header->psize >= mq_header->msgDef->queue_sz){
			if(tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mq_header->msgputWq,millisec) != osOK)
				return osErrorTimeoutResource;
		}
		tch_port_kernel_lock();
		mq_header->psize++;
		*((uint32_t*)mq_header->msgDef->pool + mq_header->pidx++) = msg;
		if(mq_header->pidx >= mq_header->msgDef->queue_sz)
			mq_header->pidx = 0;
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mq_header->msggetWq,0);
		tch_port_kernel_unlock();
		return osOK;
	}
}

osEvent tch_msgQ_get(tch_msgQue_id que_id,uint32_t millisec){
	tch_msgq_instance* mq_header = (tch_msgq_instance*) que_id;
	osEvent evt;
	evt.def.message_id = que_id;
	evt.value.v = 0;
	if(tch_port_isISR()){
		if(millisec){
			evt.status = osErrorParameter;
			return evt;
		}
		if(!mq_header->psize){
			evt.status = osEventTimeout;
			return evt;
		}
		tch_port_kernel_lock();
		evt.value.v = *((uint32_t*)mq_header->msgDef->pool + mq_header->gidx++);
		evt.status = osOK;
		if(mq_header->gidx >= mq_header->msgDef->queue_sz)
			mq_header->gidx = 0;
		tch_port_kernel_unlock();
		return evt;
	}else{
		if(!mq_header->psize){
			if(tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mq_header->msggetWq,millisec) != osOK){
				evt.status = osEventTimeout;
				return evt;
			}
		}
		tch_port_kernel_lock();
		evt.value.v = *((uint32_t*)mq_header->msgDef->pool + mq_header->gidx++);
		evt.status = osOK;
		if(mq_header->gidx >= mq_header->msgDef->queue_sz)
			mq_header->gidx = 0;
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mq_header->msggetWq,0);
		tch_port_kernel_unlock();
		return evt;
	}
}


tch_mailQue_id tch_mailQ_create(const tch_mailQueDef_t* que){

}

void* tch_mailQ_alloc(tch_mailQue_id qid,uint32_t millisec){

}

void* tch_mailQ_calloc(tch_mailQue_id qid,uint32_t millisec){

}

osStatus tch_mailQ_put(tch_mailQue_id qid){

}

osEvent tch_mailQ_get(tch_mailQue_id qid,uint32_t millisec){

}

osStatus tch_mailQ_free(tch_mailQue_id qid,void* mail){

}
