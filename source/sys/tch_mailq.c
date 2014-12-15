/*
 * tch_mailq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_ktypes.h"
#include "tch_mailq.h"

#define TCH_MAILQ_CLASS_KEY              ((uint16_t) 0x2D0D)


typedef struct _tch_mailq_cb {
	tch_uobj      __obj;
	uint32_t      bstate;         // state
	uint32_t      bsz;
	tch_mpoolId   bpool;
	tch_msgqId    msgq;
	tch_lnode_t   wq;
}tch_mailq_cb;

static tch_mailqId tch_mailq_create(uint32_t sz,uint32_t qlen);
static void* tch_mailq_alloc(tch_mailqId qid,uint32_t millisec,tchStatus* result);
static void* tch_mailq_calloc(tch_mailqId qid,uint32_t millisec,tchStatus* result);
static tchStatus tch_mailq_put(tch_mailqId qid,void* mail);
static osEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec);
static uint32_t tch_mailq_getBlockSize(tch_mailqId qid);
static uint32_t tch_mailq_getLength(tch_mailqId qid);
static tchStatus tch_mailq_free(tch_mailqId qid,void* mail);
static tchStatus tch_mailq_destroy(tch_mailqId qid);

static void tch_mailqValidate(tch_mailqId qid);
static void tch_mailqInvalidate(tch_mailqId qid);
static BOOL tch_mailqIsValid(tch_mailqId qid);

/**
 *
 * 	tch_mailqId (*create)(uint32_t sz,uint32_t qlen);
	void* (*alloc)(tch_mailqId qid,uint32_t millisec,tchStatus* result);
	void* (*calloc)(tch_mailqId qid,uint32_t millisec,tchStatus* result);
	tchStatus (*put)(tch_mailqId qid,void* mail);
	osEvent (*get)(tch_mailqId qid,uint32_t millisec);
	uint32_t (*getBlockSize)(tch_mailqId qid);
	uint32_t (*getLength)(tch_mailqId qid);
	tchStatus (*free)(tch_mailqId qid,void* mail);
	tchStatus (*destroy)(tch_mailqId qid);
 */
__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		tch_mailq_create,
		tch_mailq_alloc,
		tch_mailq_calloc,
		tch_mailq_put,
		tch_mailq_get,
		tch_mailq_getBlockSize,
		tch_mailq_getLength,
		tch_mailq_free,
		tch_mailq_destroy
};

const tch_mailq_ix* MailQ = &MailQStaticInstance;

static tch_mailqId tch_mailq_create(uint32_t sz,uint32_t qlen){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) shMem->alloc(sizeof(tch_mailq_cb));
	uStdLib->string->memset(mailqcb,0,sizeof(tch_mailq_cb));
	mailqcb->bsz = sz;
	mailqcb->__obj.destructor = (tch_uobjDestr) tch_mailq_destroy;
	mailqcb->bpool = Mempool->create(sz,qlen);
	mailqcb->msgq = MsgQ->create(qlen);
	tch_listInit(&mailqcb->wq);

	if(!(mailqcb->bpool && mailqcb->msgq)){
		shMem->free(mailqcb->bpool);
		shMem->free(mailqcb->msgq);
		return NULL;
	}

	tch_mailqValidate(mailqcb);
	return (tch_mailqId) mailqcb;
}

static void* tch_mailq_alloc(tch_mailqId qid,uint32_t millisec,tchStatus* result){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	tchStatus lresult = osOK;
	if(!result)
		result = &lresult;
	if(!tch_mailqIsValid(mailqcb))
		return NULL;
	if(tch_port_isISR()){
		return Mempool->alloc(mailqcb->bpool);
	}else{
		tch_mailq_karg karg;
		karg.timeout = millisec;
		karg.chunk = NULL;
		*result = osOK;
		while((*result = tch_port_enterSvFromUsr(SV_MAILQ_ALLOC,(uword_t)mailqcb,(uword_t)&karg)) != osOK){
			if(!tch_mailqIsValid(mailqcb))
				return NULL;
			switch(*result){
			case osErrorResource:
				return NULL;
			case osEventTimeout:
				return NULL;
			case osErrorNoMemory:
				/*signaled from no memory wait, and will retry allocation*/;
			}
		}
		if(*result == osOK)
			return karg.chunk;
		return NULL;
	}
}

tchStatus tch_mailq_kalloc(tch_mailqId qid,tch_mailq_karg* arg){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	if(!tch_mailqIsValid(mailqcb)){
		arg->chunk = NULL;
		return osErrorResource;
	}
	if((!arg) || (!qid)) return osErrorParameter;
	arg->chunk = Mempool->alloc(mailqcb->bpool);
	if(arg->chunk)
		return osOK;
	tch_schedSuspend((tch_thread_queue*)&mailqcb->wq,arg->timeout);
	return osErrorNoMemory;
}


static void* tch_mailq_calloc(tch_mailqId qid,uint32_t millisec,tchStatus* result){
	void* chunk = NULL;
	chunk = tch_mailq_alloc(qid,millisec,result);
	if(chunk){
		uStdLib->string->memset(chunk,0,((tch_mailq_cb*)qid)->bsz);
	}
	return chunk;
}

static tchStatus tch_mailq_put(tch_mailqId qid,void* mail){
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	if(!mail)
		return osErrorParameter;
	return MsgQ->put(((tch_mailq_cb*) qid)->msgq,(uword_t)mail,osWaitForever);
}


static osEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec){
	osEvent evt;
	if(!tch_mailqIsValid(qid)){
		evt.status = osErrorResource;
		evt.value.v = 0;
		return evt;
	}
	evt = MsgQ->get(((tch_mailq_cb*) qid)->msgq,millisec);
	if(evt.status == osEventMessage)
		evt.status = osEventMail;
	return evt;
}

static uint32_t tch_mailq_getBlockSize(tch_mailqId qid){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	return mailqcb->bsz;
}

static uint32_t tch_mailq_getLength(tch_mailqId qid){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	return MsgQ->getLength(mailqcb->msgq);
}

static tchStatus tch_mailq_free(tch_mailqId qid,void* mail){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	if(!tch_mailqIsValid(mailqcb))
		return osErrorResource;
	if(!mail)
		return osErrorParameter;
	if(tch_port_isISR()){
		return Mempool->free(mailqcb->bpool,mail);
	}else{
		tchStatus result = osOK;
		tch_mailq_karg karg;
		karg.chunk = mail;
		karg.timeout = 0;
		while((result = tch_port_enterSvFromUsr(SV_MAILQ_FREE,(uword_t)mailqcb,(uword_t)&karg)) != osOK){
			if(!tch_mailqIsValid(mailqcb))
				return osErrorResource;
			switch(result){
			case osErrorResource:
				return osErrorResource;
			}
		}
		return result;
	}
}

tchStatus tch_mailq_kfree(tch_mailqId qid,tch_mailq_karg* arg){
	if((!arg) || (!qid)) return osErrorParameter;
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	if(Mempool->free(((tch_mailq_cb*)qid)->bpool,arg->chunk) != osOK)
		return osErrorParameter;
	tch_schedResumeM((tch_thread_queue*) (&((tch_mailq_cb*)qid)->wq),SCHED_THREAD_ALL,osErrorNoMemory,TRUE);
	return osOK;
}


static tchStatus tch_mailq_destroy(tch_mailqId qid){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	tchStatus result = osOK;
	if(!qid)
		return osErrorParameter;
	if(!tch_mailqIsValid(mailqcb))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	if((result = tch_port_enterSvFromUsr(SV_MAILQ_DESTROY,(uword_t)qid,0)) != osOK)
		return result;
	MsgQ->destroy(mailqcb->msgq);
	shMem->free(qid);
	return osOK;
}

tchStatus tch_mailq_kdestroy(tch_mailqId qid,tch_mailq_karg* arg){
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	tchStatus result = osOK;
	result = Mempool->destroy(((tch_mailq_cb*)qid)->bpool);
	if(result == osOK){
		tch_mailqInvalidate(qid);
		tch_schedResumeM((tch_thread_queue*) (&((tch_mailq_cb*)qid)->wq) ,SCHED_THREAD_ALL,osErrorResource,TRUE);
		return osOK;
	}
	return osErrorParameter;
}

static void tch_mailqValidate(tch_mailqId qid){
	((tch_mailq_cb*) qid)->bstate |= (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}

static void tch_mailqInvalidate(tch_mailqId qid){
	((tch_mailq_cb*) qid)->bstate &= ~(0xFFFF);
}

static BOOL tch_mailqIsValid(tch_mailqId qid){
	return (((tch_mailq_cb*) qid)->bstate & 0xFFFF) == (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}
