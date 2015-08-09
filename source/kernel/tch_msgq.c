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
}tch_msgq_cb;




static tch_msgqId tch_msgq_create(uint32_t len);
static tchStatus tch_msgq_put(tch_msgqId,uint32_t msg,uint32_t millisec);
static tchEvent tch_msgq_get(tch_msgqId,uint32_t millisec);
static uint32_t tch_msgq_getLength(tch_msgqId);
static tchStatus tch_msgq_destroy(tch_msgqId);

static void tch_msgqValidate(tch_msgqId);
static void tch_msgqInvalidate(tch_msgqId);
static BOOL tch_msgqIsValid(tch_msgqId);





__attribute__((section(".data"))) static tch_msgq_ix MsgQStaticInstance = {
		tch_msgq_create,
		tch_msgq_put,
		tch_msgq_get,
		tch_msgq_getLength,
		tch_msgq_destroy
};
const tch_msgq_ix* MsgQ = &MsgQStaticInstance;


DECLARE_SYSCALL_1(messageQ_create,uint32_t,tch_msgqId);
DECLARE_SYSCALL_3(messageQ_put,tch_msgqId,uword_t,uint32_t,tchStatus);
DECLARE_SYSCALL_3(messageQ_get,tch_msgqId,tchEvent*,uint32_t,tchStatus);
DECLARE_SYSCALL_1(messageQ_destroy,tch_msgqId,tchStatus);

DEFINE_SYSCALL_1(messageQ_create,uint32_t,sz,tch_msgqId){

}


DEFINE_SYSCALL_3(messageQ_put,tch_msgqId,msgq,uword_t,msg,uint32_t,timeout,tchStatus){

}

DEFINE_SYSCALL_3(messageQ_get,tch_msgqId,msgq,tchEvent*,eventp,uint32_t,timeout,tchStatus){

	return tchEventMessage;
}

DEFINE_SYSCALL_1(messageQ_destroy,tch_msgqId,msgq,tchStatus){

}




static tch_msgqId tch_msgq_create(uint32_t len){
	if(!len)
		return NULL;
	uint8_t* msg_bptr = (uint8_t*) tch_shmAlloc(len);
	if(!msg_bptr)
		return NULL;
	return (tch_msgqId) tch_port_enterSv(SV_MSGQ_INIT,(uword_t) msg_bptr,(uword_t)len,0);
}

tch_msgqId tchk_msgqInit(uint8_t* bptr,uint32_t len){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) kmalloc(sizeof(tch_msgq_cb));
	memset(msgqCb,0,sizeof(msgqCb));

	msgqCb->bp = bptr;
	msgqCb->__obj.__destr_fn = (tch_kobjDestr) tch_msgq_destroy;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
	msgqCb->sz = len;
	msgqCb->status = 0;
	cdsl_dlistInit(&msgqCb->cwq);
	cdsl_dlistInit(&msgqCb->pwq);

	tch_msgqValidate(msgqCb);
	return (tch_msgqId) msgqCb;
}



static tchStatus tch_msgq_put(tch_msgqId mqId, uword_t msg,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tchStatus result = tchOK;
	if(!mqId || !tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(tch_port_isISR()){
		if(millisec)
			return tchErrorParameter;
		if(msgqCb->updated < msgqCb->sz){
			*((uword_t*)msgqCb->bp + msgqCb->pidx++) = msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;

			tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchOK,TRUE);
			return tchOK;
		}
		return tchErrorResource;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = msg;
		while((result = tch_port_enterSv(SV_MSGQ_PUT,(uword_t)mqId,(uword_t)&arg,0)) != tchOK){
			if(!tch_msgqIsValid(msgqCb))
				return tchErrorResource;
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
	return tchErrorOS;
}

tchStatus tchk_msgqPut(tch_msgqId mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_uheader* cth = tch_currentThread;
	if(!tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(msgqCb->updated < msgqCb->sz){
		*((uword_t*)msgqCb->bp + msgqCb->pidx++) = arg->msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;
			tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchOK,TRUE);
			return tchOK;
	}
	tchk_schedThreadSuspend((tch_thread_queue*) &msgqCb->pwq,arg->timeout);
	return tchErrorNoMemory;
}

static tchEvent tch_msgq_get(tch_msgqId mqId,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tchEvent evt;
	evt.def = mqId;
	evt.status = tchOK;
	evt.value.v = 0;
	if(!mqId || !tch_msgqIsValid(msgqCb)){
		evt.status = tchErrorResource;
		return evt;
	}
	if(tch_port_isISR()){
		if(millisec){
			evt.status = tchErrorParameter;
			return evt;
		}
		if(msgqCb->updated == 0){
			evt.status = tchErrorResource;
			return evt;
		}
		evt.value.v = *((uword_t*)msgqCb->bp + msgqCb->gidx++);
		if(msgqCb->gidx >= msgqCb->sz)
			msgqCb->gidx = 0;
		msgqCb->updated--;
		tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
		evt.status = tchOK;
		return evt;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = 0;
		while((evt.status = tch_port_enterSv(SV_MSGQ_GET,(uword_t)mqId,(uword_t)&arg,0)) != tchEventMessage){
				if(!tch_msgqIsValid(msgqCb)){
					evt.status = tchErrorResource;
					return evt;
				}
				switch(evt.status){
				case tchEventTimeout:          /// get opeartion expired given timeout
					evt.status = tchErrorTimeoutResource;
					evt.value.v = 0;
					return evt;
				case tchErrorResource:         /// msgq invalidated
					evt.value.v = 0;
					return evt;
				case tchOK:
					evt.value.v = 0;           /// wake-up from wait ( Not return )
				}
			}
		evt.value.v = arg.msg;
		return evt;
	}
	evt.status = tchErrorOS;
	return evt;
}

tchStatus tchk_msgqGet(tch_msgqId mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_uheader* cth = tch_currentThread;
	if(msgqCb->updated == 0){
		tchk_schedThreadSuspend((tch_thread_queue*) &msgqCb->cwq,arg->timeout);
		return tchOK;
	}
	arg->msg = *((uword_t*)msgqCb->bp + msgqCb->gidx++);
	if(msgqCb->gidx >= msgqCb->sz)
		msgqCb->gidx = 0;
	msgqCb->updated--;
	tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return tchEventMessage;
}

static uint32_t tch_msgq_getLength(tch_msgqId mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	return msgqCb->sz;
}


static tchStatus tch_msgq_destroy(tch_msgqId mqId){
	tch_thread_uheader* nth = NULL;
	if(tch_port_isISR())
		return tchErrorISR;
	uint8_t* bptr = tch_port_enterSv(SV_MSGQ_DEINIT,(uword_t)mqId,0,0);
	if(!bptr)
		return tchErrorResource;

	tch_shmFree(bptr);
	return tchOK;
}


void* tchk_msgqDeinit(tch_msgqId mqId){
	if(!mqId || !tch_msgqIsValid(mqId))
		return NULL;
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_uheader* nth = NULL;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
	tch_msgqInvalidate(msgqCb);
	tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedThreadResumeM((tch_thread_queue*) &msgqCb->pwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	kfree(msgqCb);
	return msgqCb->bp;
}

static inline void tch_msgqValidate(tch_msgqId mqId){
	((tch_msgq_cb*) mqId)->status |= TCH_MSGQ_CLASS_KEY ^ ((uint32_t)mqId & 0xFFFF);
}

static inline void tch_msgqInvalidate(tch_msgqId mqId){
	((tch_msgq_cb*) mqId)->status &= ~(0xFFFF);
}

static inline BOOL tch_msgqIsValid(tch_msgqId msgq){
	return (((tch_msgq_cb*)msgq)->status & 0xFFFF) == (TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF));

}



