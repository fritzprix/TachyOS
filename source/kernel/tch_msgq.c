/*
 * tch_msgq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_msgq.h"

#define TCH_MSGQ_CLASS_KEY            ((uint16_t) 0x2D03)
#define MSGQ_VALIDATE(msgq)		do{\
	((tch_msgq_cb*) msgq)->status |= TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF);\
}


typedef struct _tch_msgq_cb {
	tch_kobj      __obj;
	uint32_t      status;
	void*         bp;
	uint32_t      sz;
	uint32_t      pidx;
	uint32_t      gidx;
	cdsl_dlistNode_t   cwq;
	cdsl_dlistNode_t   pwq;
	uint32_t      updated;
}tch_msgqCb;


static tch_msgqId tch_msgq_create(uint32_t len);
static tchStatus tch_msgq_put(tch_msgqId,uint32_t msg,uint32_t millisec);
static tchEvent tch_msgq_get(tch_msgqId,uint32_t millisec);
static tchStatus tch_msgq_destroy(tch_msgqId);


static void tch_msgqValidate(tch_msgqId);
static void tch_msgqInvalidate(tch_msgqId);
static BOOL tch_msgqIsValid(tch_msgqId);


__attribute__((section(".data"))) static tch_msgq_ix MsgQStaticInstance = {
		tch_msgq_create,
		tch_msgq_put,
		tch_msgq_get,
		tch_msgq_destroy
};
const tch_msgq_ix* MsgQ = &MsgQStaticInstance;


DECLARE_SYSCALL_1(messageQ_create,uint32_t,tch_msgqId);
DECLARE_SYSCALL_3(messageQ_put,tch_msgqId,uword_t,uint32_t,tchStatus);
DECLARE_SYSCALL_3(messageQ_get,tch_msgqId,tchEvent*,uint32_t,tchStatus);
DECLARE_SYSCALL_1(messageQ_destroy,tch_msgqId,tchStatus);


DEFINE_SYSCALL_1(messageQ_create,uint32_t,sz,tch_msgqId){
	tch_msgqCb* msgqCb = (tch_msgqCb*) kmalloc(sizeof(tch_msgqCb) + sz * sizeof(uword_t));
	memset(msgqCb, 0, sizeof(tch_msgqCb));
	msgqCb->bp = (uword_t*) &msgqCb[1];
	msgqCb->__obj.__destr_fn = (tch_kobjDestr) tch_msgq_destroy;
	msgqCb->sz = sz;
	cdsl_dlistInit(&msgqCb->cwq);
	cdsl_dlistInit(&msgqCb->pwq);
	tch_msgqValidate(msgqCb);
	return (tch_msgqId) msgqCb;
}


DEFINE_SYSCALL_3(messageQ_put,tch_msgqId,msgq,uword_t,msg,uint32_t,timeout,tchStatus){
	if(!tch_msgqIsValid(msgq))
		return tchErrorParameter;
	tch_msgqCb* msgqCb = (tch_msgqCb*) msgq;
	if (msgqCb->updated < msgqCb->sz) {
		*((uword_t*) msgqCb->bp + msgqCb->pidx++) = msg;
		if (msgqCb->pidx >= msgqCb->sz)
			msgqCb->pidx = 0;
		msgqCb->updated++;
		tchk_schedWake((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL, tchErrorNoMemory, TRUE);
		return tchOK;
	}
	if(timeout)
		tchk_schedWait((tch_thread_queue*) &msgqCb->pwq,timeout);
	return tchErrorNoMemory;

}


DEFINE_SYSCALL_3(messageQ_get,tch_msgqId,msgq,tchEvent*,eventp,uint32_t,timeout,tchStatus){
	if(!msgq || !tch_msgqIsValid(msgq))
		return eventp->status = tchErrorParameter;
	tch_msgqCb* msgqcb = (tch_msgqCb*) msgq;
	if(msgqcb->updated == 0){
		if(timeout)
			tchk_schedWait((tch_thread_queue*) &msgqcb->cwq,timeout);
		return eventp->status = tchErrorResource;
	}
	eventp->value.v = ((uword_t*) msgqcb->bp)[msgqcb->gidx++];
	if(msgqcb->gidx >= msgqcb->sz)
		msgqcb->gidx = 0;
	msgqcb->updated--;
	tchk_schedWake((tch_thread_queue*) &msgqcb->cwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return eventp->status = tchEventMessage;
}


DEFINE_SYSCALL_1(messageQ_destroy,tch_msgqId,msgq,tchStatus){
	if(!msgq || !tch_msgqIsValid(msgq))
		return tchErrorParameter;
	tch_msgqCb* msgqcb = (tch_msgqCb*) msgq;
	msgqcb->updated = 0;
	tch_msgqInvalidate(msgqcb);
	tchk_schedWake((tch_thread_queue*) &msgqcb->pwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &msgqcb->cwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	kfree(msgqcb);
	return tchOK;
}


static tch_msgqId tch_msgq_create(uint32_t len){
	if(!len)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_msgqId) __SYSCALL_1(messageQ_create,len);
}


static tchStatus tch_msgq_put(tch_msgqId mqId, uword_t msg,uint32_t millisec){
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
			case tchErrorNoMemory:
				;/*  NO OP -- retry to put */
			}
		}
		return result;
	}
}


static tchEvent tch_msgq_get(tch_msgqId mqId,uint32_t millisec){
	tch_msgqCb* msgqCb = (tch_msgqCb*) mqId;
	tchEvent evt;
	evt.def = mqId;
	evt.status = tchOK;
	evt.value.v = 0;
	if(!mqId){
		evt.status = tchErrorResource;
		return evt;
	}
	if(tch_port_isISR()){
		__messageQ_get(mqId,&evt,0);
		return evt;
	}else{
		while(__SYSCALL_3(messageQ_get,mqId,&evt,millisec) != tchEventMessage){
			switch (evt.status) {
			case tchEventTimeout:        /// get opeartion expired given timeout
				evt.status = tchErrorTimeoutResource;
				evt.value.v = 0;
				return evt;
			case tchErrorResource:         /// msgq invalidated
				evt.value.v = 0;
				return evt;
			case tchErrorNoMemory:
				evt.value.v = 0;           /// wake-up from wait ( Not return )
			}
		}
		return evt;
	}
}

static tchStatus tch_msgq_destroy(tch_msgqId mqId){
	tch_thread_uheader* nth = NULL;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(messageQ_destroy,mqId);
}

static inline void tch_msgqValidate(tch_msgqId mqId){
	((tch_msgqCb*) mqId)->status |= TCH_MSGQ_CLASS_KEY ^ ((uint32_t)mqId & 0xFFFF);
}

static inline void tch_msgqInvalidate(tch_msgqId mqId){
	((tch_msgqCb*) mqId)->status &= ~(0xFFFF);
}

static inline BOOL tch_msgqIsValid(tch_msgqId msgq){
	return (((tch_msgqCb*)msgq)->status & 0xFFFF) == (TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF));

}



