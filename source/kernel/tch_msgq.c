/*
 * tch_msgq.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_msgq.h"
#include "kernel/tch_kobj.h"


#ifndef MSGQ_CLASS_KEY
#define MSGQ_CLASS_KEY            ((uint16_t) 0x2D03)
#error "might not configured properly"
#endif
#define MSGQ_VALIDATE(msgq)		do{\
	((tch_msgq_cb*) msgq)->status |= MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF);\
}

__USER_API__ static tch_msgqId tch_msgqCreate(uint32_t len);
__USER_API__ static tchStatus tch_msgqPut(tch_msgqId,uint32_t msg,uint32_t millisec);
__USER_API__ static tchEvent tch_msgqGet(tch_msgqId,uint32_t millisec);
__USER_API__ static tchStatus tch_msgqReset(tch_msgqId);
__USER_API__ static tchStatus tch_msgqDestroy(tch_msgqId);

static tch_msgqId msgq_init(tch_msgqCb* mq,uint32_t* bp,uint32_t sz,BOOL isstatic);
static tchStatus msgq_deinit(tch_msgqCb* mq);


static void tch_msgqValidate(tch_msgqId);
static void tch_msgqInvalidate(tch_msgqId);
static BOOL tch_msgqIsValid(tch_msgqId);


__USER_RODATA__ tch_messageQ_api_t MsgQ_IX = {
		.create = tch_msgqCreate,
		.put = tch_msgqPut,
		.get = tch_msgqGet,
		.reset = tch_msgqReset,
		.destroy = tch_msgqDestroy
};

__USER_RODATA__ const tch_messageQ_api_t* MsgQ = &MsgQ_IX;


DECLARE_SYSCALL_1(messageQ_create,uint32_t,tch_msgqId);
DECLARE_SYSCALL_3(messageQ_put,tch_msgqId,uword_t,uint32_t,tchStatus);
DECLARE_SYSCALL_3(messageQ_get,tch_msgqId,tchEvent*,uint32_t,tchStatus);
DECLARE_SYSCALL_1(messageQ_reset, tch_msgqId,tchStatus);
DECLARE_SYSCALL_1(messageQ_destroy,tch_msgqId,tchStatus);
DECLARE_SYSCALL_3(messageQ_init,tch_msgqCb*,uint32_t*,uint32_t,tchStatus);
DECLARE_SYSCALL_1(messageQ_deinit,tch_msgqCb*,tchStatus);

DEFINE_SYSCALL_1(messageQ_create,uint32_t,sz,tch_msgqId){
	tch_msgqCb* msgqCb = (tch_msgqCb*) kmalloc(sizeof(tch_msgqCb) + sz * sizeof(uword_t));
	return msgq_init(msgqCb,(uint32_t*) &msgqCb[1],sz,FALSE);
}


DEFINE_SYSCALL_3(messageQ_put,tch_msgqId,msgq,uword_t,msg,uint32_t,timeout,tchStatus)
{
	if(!tch_msgqIsValid(msgq))
		return tchErrorParameter;
	tch_msgqCb* msgqCb = (tch_msgqCb*) msgq;
	if (msgqCb->updated < msgqCb->sz) {
		*((uword_t*) msgqCb->bp + msgqCb->pidx++) = msg;
		if (msgqCb->pidx >= msgqCb->sz)
			msgqCb->pidx = 0;
		msgqCb->updated++;
		tch_schedWake((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL, tchInterrupted, TRUE);
		return tchOK;
	}
	if(timeout)
		tch_schedWait((tch_thread_queue*) &msgqCb->pwq,timeout);
	return tchErrorNoMemory;

}


DEFINE_SYSCALL_3(messageQ_get,tch_msgqId,msgq,tchEvent*,eventp,uint32_t,timeout,tchStatus)
{
	if(!msgq || !tch_msgqIsValid(msgq))
		return eventp->status = tchErrorParameter;
	tch_msgqCb* msgqcb = (tch_msgqCb*) msgq;
	if(msgqcb->updated == 0){
		if(timeout)
			tch_schedWait((tch_thread_queue*) &msgqcb->cwq,timeout);
		return eventp->status = tchErrorResource;
	}
	eventp->value.v = ((uword_t*) msgqcb->bp)[msgqcb->gidx++];
	if(msgqcb->gidx >= msgqcb->sz)
		msgqcb->gidx = 0;
	msgqcb->updated--;
	tch_schedWake((tch_thread_queue*) &msgqcb->cwq,SCHED_THREAD_ALL,tchInterrupted,TRUE);
	return eventp->status = tchEventMessage;
}

DEFINE_SYSCALL_1(messageQ_reset, tch_msgqId, msgq,tchStatus)
{
	if(!msgq || !tch_msgqIsValid(msgq))
		return tchErrorResource;
	tch_msgqCb* mqcb = (tch_msgqCb*) msgq;
	mqcb->gidx = 0;
	mqcb->pidx = 0;
	mqcb->updated = 0;
	tch_schedWake((tch_thread_queue*) &mqcb->cwq, SCHED_THREAD_ALL, tchInterrupted, FALSE);
	tch_schedWake((tch_thread_queue*) &mqcb->pwq, SCHED_THREAD_ALL, tchInterrupted, TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(messageQ_destroy,tch_msgqId,msgq,tchStatus)
{
	tchStatus result = tchOK;
	if((result = msgq_deinit(msgq)) != tchOK)
		return result;
	kfree(msgq);
	return result;
}

DEFINE_SYSCALL_3(messageQ_init,tch_msgqCb*,mcb,uint32_t*,bp,uint32_t,sz,tchStatus)
{
	if(!mcb || !bp || !sz)
		return tchErrorParameter;
	msgq_init(mcb,bp,sz,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(messageQ_deinit,tch_msgqCb*,mcb,tchStatus)
{
	if(!mcb || !tch_msgqIsValid(mcb))
		return tchErrorParameter;
	return msgq_deinit(mcb);
}


__USER_API__ static tch_msgqId tch_msgqCreate(uint32_t len)
{
	if(!len)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_msgqId) __SYSCALL_1(messageQ_create,len);
}


__USER_API__ static tchStatus tch_msgqPut(tch_msgqId mqId, uword_t msg,uint32_t millisec){
	tchStatus result = tchOK;
	if(!mqId)
		return tchErrorResource;
	if(tch_port_isISR()){
		return __messageQ_put(mqId,msg,0);
	}else{
		while((result = __SYSCALL_3(messageQ_put,mqId,msg,millisec)) != tchOK){
			switch(result){
			case tchEventTimeout:
				return tchErrorTimeoutResource;
			case tchErrorResource:
				return tchErrorResource;
			case tchInterrupted:
				;/*  NO OP -- retry to put */
			}
		}
		return result;
	}
}


__USER_API__ static tchEvent tch_msgqGet(tch_msgqId mqId,uint32_t millisec)
{
	tchEvent evt;
	evt.def = mqId;
	evt.status = tchOK;
	evt.value.v = 0;
	if(!mqId){
		evt.status = tchErrorResource;
		return evt;
	}
	if(tch_port_isISR())
	{
		__messageQ_get(mqId,&evt,0);
		return evt;
	}else{
		while((evt.status = __SYSCALL_3(messageQ_get,mqId,&evt,millisec)) != tchEventMessage){
			switch (evt.status) {
			case tchEventTimeout:        /// get opeartion expired given timeout
				evt.status = tchErrorTimeoutResource;
				evt.value.v = 0;
				return evt;
			case tchErrorResource:         /// msgq invalidated
				evt.value.v = 0;
				return evt;
			case tchInterrupted:
				evt.value.v = 0;           /// wake-up from wait ( Not return )
			}
		}
		return evt;
	}
}

__USER_API__ static tchStatus tch_msgqReset(tch_msgqId mqId)
{
	if(tch_port_isISR())
	{
		return __messageQ_reset(mqId);
	}
	return __SYSCALL_1(messageQ_reset, mqId);
}

__USER_API__ static tchStatus tch_msgqDestroy(tch_msgqId mqId)
{
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(messageQ_destroy,mqId);
}


static tch_msgqId msgq_init(tch_msgqCb* mq,uint32_t* bp,uint32_t sz,BOOL isstatic){
	mset(mq, 0, sizeof(tch_msgqCb));
	mq->bp = bp;
	mq->sz = sz;
	cdsl_dlistEntryInit(&mq->cwq);
	cdsl_dlistEntryInit(&mq->pwq);
	tch_registerKobject(&mq->__obj,isstatic? (tch_kobjDestr) tch_msgqDeinit : (tch_kobjDestr) tch_msgqDestroy);
	tch_msgqValidate(mq);
	return mq;
}


static tchStatus msgq_deinit(tch_msgqCb* mq){
	if (!mq || !tch_msgqIsValid(mq))
		return tchErrorParameter;
	mq->updated = 0;
	tch_msgqInvalidate(mq);
	tch_schedWake((tch_thread_queue*) &mq->pwq, SCHED_THREAD_ALL,tchErrorResource, FALSE);
	tch_schedWake((tch_thread_queue*) &mq->cwq, SCHED_THREAD_ALL,tchErrorResource, FALSE);
	tch_unregisterKobject(&mq->__obj);
	return tchOK;
}


tchStatus tch_msgqInit(tch_msgqCb* mq,uint32_t* bp,uint32_t sz){
	if(!mq || !bp || !sz)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __messageQ_init(mq,bp,sz);
	return __SYSCALL_3(messageQ_init,mq,bp,sz);
}

tchStatus tch_msgqDeinit(tch_msgqCb* mq){
	if(!mq)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __messageQ_deinit(mq);
	return __SYSCALL_1(messageQ_deinit,mq);
}


static inline void tch_msgqValidate(tch_msgqId mqId){
	((tch_msgqCb*) mqId)->status |= MSGQ_CLASS_KEY ^ ((uint32_t)mqId & 0xFFFF);
}

static inline void tch_msgqInvalidate(tch_msgqId mqId){
	((tch_msgqCb*) mqId)->status &= ~(0xFFFF);
}

static inline BOOL tch_msgqIsValid(tch_msgqId msgq){
	return (((tch_msgqCb*)msgq)->status & 0xFFFF) == (MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF));

}



