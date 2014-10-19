/*
 * tch_msgq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_sys.h"
#include "tch_msgq.h"

#define TCH_MSGQ_CLASS_KEY            ((uint16_t) 0x2D03)
#define tch_msgqValidate(msgq)        ((tch_msgq_cb*)msgq)->state = TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF)
#define tch_msgqInvalidate(msgq)      ((tch_msgq_cb*)msgq)->state &= ~(0xFFFF)
//#define tch_msgqIsValid(msgq)         (((tch_msgq_cb*)msgq)->state & 0xFFFF) == (TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF))



typedef struct _tch_msgq_cb {
	uint32_t state;
	void*         bp;
	size_t        sz;
	uint32_t      pidx;
	uint32_t      gidx;
	tch_lnode_t   cwq;
	tch_lnode_t   pwq;
	size_t        updated;
}tch_msgq_cb;



tchStatus tch_msgq_kput(tch_msgQue_id,tch_msgq_karg* arg);
tchStatus tch_msgq_kget(tch_msgQue_id,tch_msgq_karg* arg);
tchStatus tch_msgq_kdestroy(tch_msgQue_id);

static tch_msgQue_id tch_msgq_createApi(size_t len);
static tchStatus tch_msgq_putApi(tch_msgQue_id,uint32_t msg,uint32_t millisec);
static osEvent tch_msgq_getApi(tch_msgQue_id,uint32_t millisec);
static tchStatus tch_msgq_destroyApi(tch_msgQue_id);
static BOOL tch_msgqIsValid(tch_msgQue_id);





__attribute__((section(".data"))) static tch_msgq_ix MsgQStaticInstance = {
		tch_msgq_createApi,
		tch_msgq_putApi,
		tch_msgq_getApi,
		tch_msgq_destroyApi
};
const tch_msgq_ix* MsgQ = &MsgQStaticInstance;


static tch_msgQue_id tch_msgq_createApi(size_t len){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) Mem->alloc(sizeof(tch_msgq_cb) + len * sizeof(uaddr_t));
	msgqCb->bp = (tch_msgq_cb*) msgqCb + 1;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
	msgqCb->sz = len;
	tch_listInit(&msgqCb->cwq);
	tch_listInit(&msgqCb->pwq);

	tch_msgqValidate(msgqCb);
	return (tch_msgQue_id) msgqCb;
}



static tchStatus tch_msgq_putApi(tch_msgQue_id mqId, uword_t msg,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tchStatus result = osOK;
	if(!tch_msgqIsValid(msgqCb))
		return osErrorResource;
	if(tch_port_isISR()){
		if(millisec)
			return osErrorParameter;
		if(msgqCb->updated < msgqCb->sz){
			*((uword_t*)msgqCb->bp + msgqCb->pidx++) = msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;

			//tch_schedResumeAll(&msgqCb->cwq,osEventMessage);
			tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osOK,TRUE);
			return osOK;
		}
		return osErrorResource;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = msg;
		while((result = tch_port_enterSvFromUsr(SV_MSGQ_PUT,(uword_t)mqId,(uword_t)&arg)) != osOK){
			if(!tch_msgqIsValid(msgqCb))
				return osErrorResource;
			switch(result){
			case osEventTimeout:
				return osErrorTimeoutResource;
			case osErrorResource:
				return osErrorResource;
			case osErrorNoMemory:
				;/*  NO OP -- retry to put */
			}
		}
		return result;
	}
	return osErrorOS;
}

tchStatus tch_msgq_kput(tch_msgQue_id mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* cth = tch_currentThread;
	if(!tch_msgqIsValid(msgqCb))
		return osErrorResource;
	if(msgqCb->updated < msgqCb->sz){
		*((uword_t*)msgqCb->bp + msgqCb->pidx++) = arg->msg;
			if(msgqCb->pidx >= msgqCb->sz)
				msgqCb->pidx = 0;
			msgqCb->updated++;
			//tch_schedResumeAll(&msgqCb->cwq,osOK,TRUE);
			tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osOK,TRUE);
			return osOK;
	}
	tch_schedSuspend(&msgqCb->pwq,arg->timeout);
	return osErrorNoMemory;
}


static osEvent tch_msgq_getApi(tch_msgQue_id mqId,uint32_t millisec){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	osEvent evt;
	evt.def = mqId;
	evt.status = osOK;
	evt.value.v = 0;
	if(!tch_msgqIsValid(msgqCb)){
		evt.status = osErrorResource;
		return evt;
	}
	if(tch_port_isISR()){
		if(millisec){
			evt.status = osErrorParameter;
			return evt;
		}
		if(msgqCb->updated == 0){
			evt.status = osErrorResource;
			return evt;
		}
		evt.value.v = *((uword_t*)msgqCb->bp + msgqCb->gidx++);
		if(msgqCb->gidx >= msgqCb->sz)
			msgqCb->gidx = 0;
		msgqCb->updated--;
		//tch_schedResumeAll(&msgqCb->pwq,osErrorNoMemory,TRUE);
		tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osErrorNoMemory,TRUE);
		evt.status = osOK;
		return evt;
	}else{
		tch_msgq_karg arg;
		arg.timeout = millisec;
		arg.msg = 0;
		while((evt.status = tch_port_enterSvFromUsr(SV_MSGQ_GET,(uword_t)mqId,(uword_t)&arg)) != osEventMessage){
				if(!tch_msgqIsValid(msgqCb)){
					evt.status = osErrorResource;
					return evt;
				}
				switch(evt.status){
				case osEventTimeout:          /// get opeartion expired given timeout
					evt.status = osErrorTimeoutResource;
					evt.value.v = 0;
					return evt;
				case osErrorResource:         /// msgq invalidated
					evt.value.v = 0;
					return evt;
				case osOK:
					evt.value.v = 0;           /// wake-up from wait ( Not return )
				}
			}
		evt.value.v = arg.msg;
		return evt;
	}
	evt.status = osErrorOS;
	return evt;
}

tchStatus tch_msgq_kget(tch_msgQue_id mqId,tch_msgq_karg* arg){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* cth = tch_currentThread;
	if(!tch_msgqIsValid(msgqCb))
		return osErrorResource;
	if(msgqCb->updated == 0){
		tch_schedSuspend(&msgqCb->cwq,arg->timeout);
		return osOK;
	}
	arg->msg = *((uword_t*)msgqCb->bp + msgqCb->gidx++);
	if(msgqCb->gidx >= msgqCb->sz)
		msgqCb->gidx = 0;
	msgqCb->updated--;
//	tch_schedResumeAll(&msgqCb->pwq,osErrorNoMemory,TRUE);
	tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osErrorNoMemory,TRUE);
	return osEventMessage;
}

static tchStatus tch_msgq_destroyApi(tch_msgQue_id mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* nth = NULL;
	if(!tch_msgqIsValid(msgqCb))
		return osErrorResource;
	if(tch_port_isISR()){
		tch_msgqInvalidate(msgqCb);
		msgqCb->bp = NULL;
		msgqCb->gidx = 0;
		msgqCb->pidx = 0;
	/*	tch_schedResumeAll(&msgqCb->pwq,osErrorResource,FALSE);
		tch_schedResumeAll(&msgqCb->cwq,osErrorResource,TRUE);  */
		tch_schedResumeM(&msgqCb->pwq,SCHED_THREAD_ALL,osErrorResource,FALSE);
		tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osErrorResource,TRUE);
		msgqCb->updated = 0;
		Mem->free(msgqCb);
	}else{
		tch_port_enterSvFromUsr(SV_MSGQ_DESTROY,(uword_t)mqId,0);
		Mem->free(msgqCb);
	}
	return osOK;
}

static BOOL tch_msgqIsValid(tch_msgQue_id msgq){
	return (((tch_msgq_cb*)msgq)->state & 0xFFFF) == (TCH_MSGQ_CLASS_KEY ^ ((uint32_t)msgq & 0xFFFF));

}


tchStatus tch_msgq_kdestroy(tch_msgQue_id mqId){
	tch_msgq_cb* msgqCb = (tch_msgq_cb*) mqId;
	tch_thread_header* nth = NULL;
	msgqCb->gidx = 0;
	msgqCb->pidx = 0;
	msgqCb->updated = 0;
/*	tch_schedResumeAll(&msgqCb->cwq,osErrorResource,FALSE);
	tch_schedResumeAll(&msgqCb->pwq,osErrorResource,TRUE);*/
	tch_schedResumeM(&msgqCb->cwq,SCHED_THREAD_ALL,osErrorResource,FALSE);
	tch_schedResumeM(&msgqCb->pwq,SCHED_THREAD_ALL,osErrorResource,TRUE);
	return osOK;
}

