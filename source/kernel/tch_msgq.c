/*
 * tch_msgq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_msgq.h"

#define TCH_MSGQ_CLASS_KEY            ((uint16_t) 0x2D03)

typedef struct _tch_msgq_cb {
	tch_uobj      __obj;
	uint32_t      status;
	void*         bp;
	uint32_t      sz;
	uint32_t      pidx;
	uint32_t      gidx;
	tch_lnode_t   cwq;
	tch_lnode_t   pwq;
	uint32_t      updated;
}tch_msgq_cb;



tchStatus tch_msgq_kput(tch_msgqId,tch_msgq_karg* arg);
tchStatus tch_msgq_kget(tch_msgqId,tch_msgq_karg* arg);
tchStatus tch_msgq_kdestroy(tch_msgqId);

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


static tch_msgqId tch_msgq_create(uint32_t len){
	size_t sz = sizeof(tch_msgq_cb) + len * sizeof(uaddr_t);
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) shMem->alloc(sz);
	uStdLib->string->memset(msgqCb,0,sz);

	msgqCb->bp = (tch_msgq_cb*) msgqCb + 1;
	msgqCb->__obj.destructor = (tch_uobjDestr) tch_msgq_destroy;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
	msgqCb->sz = len;
	msgqCb->status = 0;
	tch_listInit(&msgqCb->cwq);
	tch_listInit(&msgqCb->pwq);

	tch_msgqValidate(msgqCb);
	return (tch_msgqId) msgqCb;
}



static tchStatus tch_msgq_put(tch_msgqId mqId, uword_t msg,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tchStatus result = tchOK;
	if(!tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(tch_port_isISR()){
		if(millisec)
			return tchErrorParameter;
		if(msgqCb->updated < msgqCb->sz){
			*((uword_t*)msgqCb->bp + msgqCb->pidx++) = msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;

			tch_schedResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchOK,TRUE);
			return tchOK;
		}
		return tchErrorResource;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = msg;
		while((result = tch_port_enterSv(SV_MSGQ_PUT,(uword_t)mqId,(uword_t)&arg)) != tchOK){
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

tchStatus tch_msgq_kput(tch_msgqId mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* cth = tch_currentThread;
	if(!tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(msgqCb->updated < msgqCb->sz){
		*((uword_t*)msgqCb->bp + msgqCb->pidx++) = arg->msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;
			tch_schedResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchOK,TRUE);
			return tchOK;
	}
	tch_schedSuspendThread((tch_thread_queue*) &msgqCb->pwq,arg->timeout);
	return tchErrorNoMemory;
}


static tchEvent tch_msgq_get(tch_msgqId mqId,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tchEvent evt;
	evt.def = mqId;
	evt.status = tchOK;
	evt.value.v = 0;
	if(!tch_msgqIsValid(msgqCb)){
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
		tch_schedResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
		evt.status = tchOK;
		return evt;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = 0;
		while((evt.status = tch_port_enterSv(SV_MSGQ_GET,(uword_t)mqId,(uword_t)&arg)) != tchEventMessage){
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

tchStatus tch_msgq_kget(tch_msgqId mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* cth = tch_currentThread;
	if(!tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(msgqCb->updated == 0){
		tch_schedSuspendThread((tch_thread_queue*) &msgqCb->cwq,arg->timeout);
		return tchOK;
	}
	arg->msg = *((uword_t*)msgqCb->bp + msgqCb->gidx++);
	if(msgqCb->gidx >= msgqCb->sz)
		msgqCb->gidx = 0;
	msgqCb->updated--;
	tch_schedResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return tchEventMessage;
}

static uint32_t tch_msgq_getLength(tch_msgqId mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	return msgqCb->sz;
}


static tchStatus tch_msgq_destroy(tch_msgqId mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* nth = NULL;
	if(!tch_msgqIsValid(msgqCb))
		return tchErrorResource;
	if(tch_port_isISR())
		return tchErrorISR;
	tch_port_enterSv(SV_MSGQ_DESTROY,(uword_t)mqId,0);
	tch_msgqInvalidate(msgqCb);
	shMem->free(msgqCb);
	return tchOK;
}


tchStatus tch_msgq_kdestroy(tch_msgqId mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* nth = NULL;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
	tch_schedResumeM((tch_thread_queue*) &msgqCb->cwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tch_schedResumeM((tch_thread_queue*) &msgqCb->pwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	return tchOK;
}

static void tch_msgqValidate(tch_msgqId mqId){
	((tch_msgq_cb*) mqId)->status |= TCH_MSGQ_CLASS_KEY ^ ((uint32_t)mqId & 0xFFFF);
}

static void tch_msgqInvalidate(tch_msgqId mqId){
	((tch_msgq_cb*) mqId)->status &= ~(0xFFFF);
}

static BOOL tch_msgqIsValid(tch_msgqId msgq){
	return (((tch_msgq_cb*)msgq)->status & 0xFFFF) == (TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF));

}



