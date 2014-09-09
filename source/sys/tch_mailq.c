/*
 * tch_mailq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_sys.h"
#include "tch_ktypes.h"
#include "tch_mailq.h"

#define TCH_MAILQ_CLASS_KEY              ((uint16_t) 0x2D0D)
#define tch_mailqValidate(mailq)         ((tch_mailq_cb*) mailq)->bstate = (((uint32_t) mailq & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
#define tch_mailqInvalidate(mailq)       ((tch_mailq_cb*) mailq)->bstate &= ~(0xFFFF)
#define tch_mailqIsValid(mailq)          ((((tch_mailq_cb*) mailq)->bstate & 0xFFFF) == (((uint32_t) mailq & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY))




typedef struct _tch_mailq_cb {
	uint32_t      bstate;
	void*         bp;
	size_t        bsz;
	size_t        blen;
	size_t        updated;
	size_t        pidx;
	size_t        gidx;
	tch_mpoolId   bpool;
	tch_msgQue_id msgq;
	tch_lnode_t   wq;
}tch_mailq_cb;


static tch_mailQue_id tch_mailq_create(size_t sz,uint32_t qlen);
static void* tch_mailq_alloc(tch_mailQue_id qid,uint32_t millisec,tchStatus* result);
static void* tch_mailq_calloc(tch_mailQue_id qid,uint32_t millisec,tchStatus* result);
static tchStatus tch_mailq_put(tch_mailQue_id qid,void* mail);
static osEvent tch_mailq_get(tch_mailQue_id qid,uint32_t millisec);
static tchStatus tch_mailq_free(tch_mailQue_id qid,void* mail);
static tchStatus tch_mailq_destroy(tch_mailQue_id qid);


__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		tch_mailq_create,
		tch_mailq_alloc,
		tch_mailq_calloc,
		tch_mailq_put,
		tch_mailq_get,
		tch_mailq_free,
		tch_mailq_destroy
};

const tch_mailq_ix* MailQ = &MailQStaticInstance;

static tch_mailQue_id tch_mailq_create(size_t sz,uint32_t qlen){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) Mem->alloc(sizeof(tch_mailq_cb) + sizeof(void*) * qlen);
	mailqcb->bp = ((tch_mailq_cb*) mailqcb + 1);
	mailqcb->blen = qlen;
	mailqcb->bsz = sz;
	mailqcb->pidx = 0;
	mailqcb->gidx = 0;
	mailqcb->updated = 0;

	mailqcb->bpool = Mempool->create(sz,qlen);
	mailqcb->msgq = MsgQ->create(qlen);
	tch_listInit(&mailqcb->wq);

	if(!(mailqcb->bpool && mailqcb->msgq)){
		Mem->free(mailqcb->bpool);
		Mem->free(mailqcb->msgq);
		return NULL;
	}

	tch_mailqValidate(mailqcb);
	return (tch_mailQue_id) mailqcb;
}

static void* tch_mailq_alloc(tch_mailQue_id qid,uint32_t millisec,tchStatus* result){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	tchStatus lresult = osOK;
	if(!result)
		result = &lresult;
	if(tch_port_isISR()){
		if(!tch_mailqIsValid(mailqcb))
			return NULL;
		return Mempool->alloc(mailqcb->bpool);
	}else{
		tch_mailq_karg karg;
		karg.timeout = millisec;
		karg.chunk = NULL;
		*result = osOK;
		while((*result = tch_port_enterSvFromUsr(SV_MAILQ_ALLOC,mailqcb,&karg)) != osOK){
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

tchStatus tch_mailq_kalloc(tch_mailQue_id qid,tch_mailq_karg* arg){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	if(!tch_mailqIsValid(mailqcb)){
		arg->chunk = NULL;
		return osErrorResource;
	}
	if((!arg) || (!qid)) return osErrorParameter;
	arg->chunk = Mempool->alloc(mailqcb->bpool);
	if(arg->chunk)
		return osOK;
	tch_schedSuspend(&mailqcb->wq,arg->timeout);
	return osErrorNoMemory;
}


static void* tch_mailq_calloc(tch_mailQue_id qid,uint32_t millisec,tchStatus* result){
	void* chunk = NULL;
	chunk = tch_mailq_alloc(qid,millisec,result);
	if(chunk){
		Sys->tch_api.uStdLib->string->memset(chunk,0,((tch_mailq_cb*)qid)->bsz);
	}
	return chunk;
}

static tchStatus tch_mailq_put(tch_mailQue_id qid,void* mail){
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	if(!mail)
		return osErrorParameter;
	return MsgQ->put(((tch_mailq_cb*) qid)->msgq,mail,osWaitForever);
}


static osEvent tch_mailq_get(tch_mailQue_id qid,uint32_t millisec){
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


static tchStatus tch_mailq_free(tch_mailQue_id qid,void* mail){
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
		while((result = tch_port_enterSvFromUsr(SV_MAILQ_FREE,mailqcb,(uword_t)&karg)) != osOK){
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

tchStatus tch_mailq_kfree(tch_mailQue_id qid,tch_mailq_karg* arg){
	if((!arg) || (!qid)) return osErrorParameter;
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	if(Mempool->free(((tch_mailq_cb*)qid)->bpool,arg->chunk) != osOK)
		return osErrorParameter;
	tch_schedResumeAll(&((tch_mailq_cb*)qid)->wq,osErrorNoMemory);
	return osOK;
}


static tchStatus tch_mailq_destroy(tch_mailQue_id qid){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	tchStatus result = osOK;
	if(!qid)
		return osErrorParameter;
	if(!tch_mailqIsValid(mailqcb))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	if((result = tch_port_enterSvFromUsr(SV_MAILQ_DESTROY,qid,0)) != osOK)
		return result;
	MsgQ->destroy(mailqcb->msgq);
	return osOK;
}

tchStatus tch_mailq_kdestroy(tch_mailQue_id qid,tch_mailq_karg* arg){
	if(!tch_mailqIsValid(qid))
		return osErrorResource;
	tchStatus result = osOK;
	result = Mempool->destroy(((tch_mailq_cb*)qid)->bpool);
	if(result == osOK){
		tch_mailqInvalidate(qid);
		tch_schedResumeAll(&((tch_mailq_cb*)qid)->wq,osErrorResource,TRUE);
		return osOK;
	}
	return osErrorParameter;
}

