/*
 * tch_ipc.c
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

#include <stdlib.h>

#include "tch_kernel.h"



typedef struct tch_msgq_instance {
	uint32_t                      pidx;
	uint32_t                      gidx;
	uint32_t                      psize;
	const tch_msgQueDef_t*        msgDef;
	tch_lnode_t                   msgputWq;
	tch_lnode_t                   msggetWq;
}tch_msgq_instance;

/***
 *  MailQueue
 */
typedef struct tch_mailq_instance {
	uint32_t                      pidx;
	uint32_t                      gidx;
	uint32_t                      psize;
	void*                         bfree;
	void*                         bend;
	tch_lnode_t                   mailGetWq;
	tch_lnode_t                   mailAllocWq;
	tch_mailQueDef_t*             mailqDef;
}tch_mailq_instance;


static tch_msgQue_id tch_msgQ_create(const tch_msgQueDef_t* que);
static tchStatus tch_msgQ_put(tch_msgQue_id,uint32_t msg,uint32_t millisec);
static osEvent tch_msgQ_get(tch_msgQue_id,uint32_t millisec);
static tchStatus tch_msgQ_destroy(tch_msgQue_id id);


static tch_mailQue_id tch_mailQ_create(const tch_mailQueDef_t* que);
static void* tch_mailQ_alloc(tch_mailQue_id qid,uint32_t millisec);
static void* tch_mailQ_calloc(tch_mailQue_id qid,uint32_t millisec);
static tchStatus tch_mailQ_put(tch_mailQue_id qid,void* mail);
static osEvent tch_mailQ_get(tch_mailQue_id qid,uint32_t millisec);
static tchStatus tch_mailQ_free(tch_mailQue_id qid,void* mail);



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

static tch_msgQue_id tch_msgQ_create(const tch_msgQueDef_t* que){
	memset(que->pool,0,que->item_sz * que->queue_sz);
	tch_msgq_instance* msgq_header = (tch_msgq_instance*) que->pool - 1;            ///< msgq header is located in lowest address of pool.
	msgq_header->pidx = 0;
	msgq_header->gidx = 0;
	msgq_header->psize = 0;
	msgq_header->msgDef = que;                                                      ///< initialize msgq header
	tch_listInit(&msgq_header->msgputWq);                                    ///< initialize wait queue for thread blocked in put function
	tch_listInit(&msgq_header->msggetWq);                                    ///< initialize wait queue for thread blocked in get function
	return (tch_msgQue_id) msgq_header;
}

static tchStatus tch_msgQ_put(tch_msgQue_id que_id,uint32_t msg,uint32_t millisec){
	tch_msgq_instance* mq_header = (tch_msgq_instance*) que_id;
	if(tch_port_isISR()){                                                            ///< ISR Mode
		if(millisec)                                                                 ///< ##Note : in ISR Mode, timeout is taken into error, because isr mode execution can not blocked
			return osErrorParameter;                                                 ///<          and it generate 'osErrorParameter'
		if(mq_header->psize >= mq_header->msgDef->queue_sz)
			return osErrorResource;
		tch_port_kernel_lock();                                                      ///< ensure not to be preempted
		mq_header->psize++;
		*((uint32_t*)mq_header->msgDef->pool + mq_header->pidx++) = msg;
		if(mq_header->pidx >= mq_header->msgDef->queue_sz)
			mq_header->pidx = 0;
		tch_port_kernel_unlock();
		return osOK;
	}else{                                                                            ///< Thread Mode
		if(mq_header->psize >= mq_header->msgDef->queue_sz){                          ///< check availability of queue
			if(tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mq_header->msgputWq,millisec) != osOK)   ///< if there is no memory in the queue, thread will be blocked
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

static osEvent tch_msgQ_get(tch_msgQue_id que_id,uint32_t millisec){
	tch_msgq_instance* mq_header = (tch_msgq_instance*) que_id;
	osEvent evt;
	evt.def = que_id;
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
		mq_header->psize--;
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
		mq_header->psize--;
		evt.value.v = *((uint32_t*)mq_header->msgDef->pool + mq_header->gidx++);
		evt.status = osOK;
		if(mq_header->gidx >= mq_header->msgDef->queue_sz)
			mq_header->gidx = 0;
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mq_header->msggetWq,0);
		tch_port_kernel_unlock();
		return evt;
	}
}

static tchStatus tch_msgQ_destroy(tch_msgQue_id id){
	return osErrorOS;
}



static tch_mailQue_id tch_mailQ_create(const tch_mailQueDef_t* que){
	memset(que->pool,0,que->item_sz * que->queue_sz);
	tch_mailq_instance* mailq = (tch_mailq_instance*) que->pool - 1;
	mailq->bfree = que->pool;
	mailq->bend = ((uint8_t*) que->pool + que->item_sz * que->queue_sz);          ///< assign end of pool
	void* end = ((uint8_t*)mailq->bend - que->item_sz);                            ///< temporal end to initiate list
	void* next = NULL;
	void* blk = mailq->bfree;
	while(1){
		next = ((uint8_t*)blk + que->item_sz);
		if(next > end) break;
		*(void**)blk = next;                      ///< assign vector for next block
		blk = next;                       ///< move index to next
	}
	*(void**) blk = 0;                             ///< mempool for mailQ initialized

	mailq->gidx = 0;
	mailq->pidx = 0;
	mailq->psize = 0;
	mailq->mailqDef = (tch_mailQueDef_t*)que;
	tch_listInit(&mailq->mailAllocWq);
	tch_listInit(&mailq->mailGetWq);
	return mailq;
}

static void* tch_mailQ_alloc(tch_mailQue_id qid,uint32_t millisec){
	if(tch_port_isISR()){
		millisec = 0;
	}
	tchStatus res = osOK;
	tch_mailq_instance* mailq = (tch_mailq_instance*) qid;
	if(mailq->bfree){                                           //< if there is no more memory to allocate, block current thread execution
		res = (tchStatus)tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mailq->mailAllocWq,millisec);
	}
	if((res == osErrorTimeoutResource))      //< check kernel call result
		return NULL;
	tch_port_kernel_lock();
	void** free = (void**) mailq->bfree;
	if(free){
		mailq->bfree = *free;
	}
	tch_port_kernel_unlock();
	return free;
}

static void* tch_mailQ_calloc(tch_mailQue_id qid,uint32_t millisec){
	if(tch_port_isISR()){
		millisec = 0;
	}
	tchStatus res = osOK;
	tch_mailq_instance* mailq = (tch_mailq_instance*) qid;
	if(!mailq->bfree){                                           //< if there is no more memory to allocate, block current thread execution
		res = (tchStatus)tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mailq->mailAllocWq,millisec);
	}
	if((res == osErrorTimeoutResource))      //< check kernel call result
		return NULL;
	tch_port_kernel_lock();
	void** free = (void**)mailq->bfree;
	if(free){
		mailq->bfree = *free;
	}
	tch_port_kernel_unlock();
	memset(free,0,mailq->mailqDef->item_sz);
	return free;
}



static tchStatus tch_mailQ_free(tch_mailQue_id qid,void* mail){
	tch_mailq_instance* mailq = (tch_mailq_instance*) qid;
	if((mailq->mailqDef->pool > mail) && (mailq->bend <= mail))
		return osErrorValue;
	tch_port_kernel_lock();
	*(void**)mail = mailq->bfree;
	mailq->bfree = mail;
	if(tch_port_isISR()){
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&mailq->mailAllocWq,0);
	}else{
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mailq->mailAllocWq,0);
	}
	tch_port_kernel_unlock();
	return osOK;
}

static osEvent tch_mailQ_get(tch_mailQue_id qid,uint32_t millisec){
	tch_mailq_instance* mailq = (tch_mailq_instance*) qid;
	osEvent evt;
	evt.status = osOK;
	evt.def = qid;
	evt.value.v = 0;
	if(tch_port_isISR())
		millisec = 0;
	if((!mailq->psize) && millisec){
		evt.status = (tchStatus) tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&mailq->mailGetWq,millisec);
	}
	if(evt.status == osErrorTimeoutResource){
		evt.status = osEventTimeout;
		return evt;
	}
	tch_port_kernel_lock();
	mailq->psize--;
	evt.value.v = *((uint32_t*)mailq->mailqDef->queue + mailq->gidx++);
	if(mailq->gidx >= mailq->mailqDef->queue_sz)
		mailq->gidx = 0;
	evt.status = osEventMail;
	tch_port_kernel_unlock();
	return evt;
}


static tchStatus tch_mailQ_put(tch_mailQue_id qid,void* mail){
	tch_mailq_instance* mailq = (tch_mailq_instance*) qid;
	tch_port_kernel_lock();
	mailq->psize++;
	*((uint32_t*) mailq->mailqDef->queue + mailq->pidx++) = (uint32_t) mail;
	if(mailq->pidx >= mailq->mailqDef->queue_sz)
		mailq->pidx = 0;
	if(tch_port_isISR()){
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&mailq->mailGetWq,0);
	}else{
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&mailq->mailGetWq,0);
	}
	tch_port_kernel_unlock();
	return osOK;
}

