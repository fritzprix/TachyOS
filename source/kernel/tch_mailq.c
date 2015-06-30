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


typedef struct tch_mailqCb {
	tch_kobj      __obj;
	uint32_t      bstatus;         // state
	uint32_t      bsz;
	tch_mpoolId   bpool;
	tch_msgqId    msgq;
	cdsl_dlistNode_t   wq;
}tch_mailqCb;


static tch_mailqId tch_mailq_create(uint32_t sz,uint32_t qlen);
static void* tch_mailq_alloc(tch_mailqId qid,uint32_t millisec,tchStatus* result);
static void* tch_mailq_calloc(tch_mailqId qid,uint32_t millisec,tchStatus* result);
static tchStatus tch_mailq_put(tch_mailqId qid,void* mail);
static tchEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec);
static uint32_t tch_mailq_getBlockSize(tch_mailqId qid);
static uint32_t tch_mailq_getLength(tch_mailqId qid);
static tchStatus tch_mailq_free(tch_mailqId qid,void* mail);
static tchStatus tch_mailq_destroy(tch_mailqId qid);

static void tch_mailqValidate(tch_mailqId qid);
static void tch_mailqInvalidate(tch_mailqId qid);
static BOOL tch_mailqIsValid(tch_mailqId qid);

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
	tch_mailqCb* mailqcb = (tch_mailqCb*) tch_shMemAlloc(sizeof(tch_mailqCb),TRUE);
	tch_mailqCb umailq;
	memset(&umailq,0,sizeof(tch_mailqCb));
	umailq.bpool = Mempool->create(sz,qlen);
	umailq.msgq = MsgQ->create(qlen);
	if(!umailq.bpool || !umailq.msgq){
		Mempool->destroy(umailq.bpool);
		MsgQ->destroy(umailq.msgq);
		return NULL;
	}

	umailq.__obj.__destr_fn = (tch_kobjDestr) tch_mailq_destroy;
	umailq.bsz = sz;
	cdsl_dlistInit(&umailq.wq);
	return tch_port_enterSv(SV_MAILQ_INIT,mailqcb,&umailq);
}

tch_mailqId tch_mailqInit(tch_mailqId qdest,tch_mailqId qsrc){
	memcpy(qdest,qsrc,sizeof(tch_mailqCb));
	tch_mailqValidate(qdest);
	return qdest;
}


static void* tch_mailq_alloc(tch_mailqId qid,uint32_t millisec,tchStatus* result){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	tchStatus lresult = tchOK;
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
		*result = tchOK;
		while((*result = tch_port_enterSv(SV_MAILQ_ALLOC,(uword_t)mailqcb,(uword_t)&karg)) != tchOK){
			if(!tch_mailqIsValid(mailqcb))
				return NULL;
			switch(*result){
			case tchErrorResource:
				return NULL;
			case tchEventTimeout:
				return NULL;
			case tchErrorNoMemory:
				/*signaled from no memory wait, and will retry allocation*/;
			}
		}
		if(*result == tchOK)
			return karg.chunk;
		return NULL;
	}
}

tchStatus tchk_mailqAlloc(tch_mailqId qid,tch_mailq_karg* arg){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	if(!tch_mailqIsValid(mailqcb)){
		arg->chunk = NULL;
		return tchErrorResource;
	}
	if((!arg) || (!qid)) return tchErrorParameter;
	arg->chunk = Mempool->alloc(mailqcb->bpool);
	if(arg->chunk)
		return tchOK;
	tchk_schedThreadSuspend((tch_thread_queue*)&mailqcb->wq,arg->timeout);
	return tchErrorNoMemory;
}


static void* tch_mailq_calloc(tch_mailqId qid,uint32_t millisec,tchStatus* result){
	void* chunk = NULL;
	chunk = tch_mailq_alloc(qid,millisec,result);
	if(chunk){
		memset(chunk,0,((tch_mailqCb*)qid)->bsz);
	}
	return chunk;
}

static tchStatus tch_mailq_put(tch_mailqId qid,void* mail){
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	if(!mail)
		return tchErrorParameter;
	return MsgQ->put(((tch_mailqCb*) qid)->msgq,(uword_t)mail,tchWaitForever);
}


static tchEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec){
	tchEvent evt;
	if(!tch_mailqIsValid(qid)){
		evt.status = tchErrorResource;
		evt.value.v = 0;
		return evt;
	}
	evt = MsgQ->get(((tch_mailqCb*) qid)->msgq,millisec);
	if(evt.status == tchEventMessage)
		evt.status = tchEventMail;
	return evt;
}

static uint32_t tch_mailq_getBlockSize(tch_mailqId qid){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	return mailqcb->bsz;
}

static uint32_t tch_mailq_getLength(tch_mailqId qid){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	return MsgQ->getLength(mailqcb->msgq);
}

static tchStatus tch_mailq_free(tch_mailqId qid,void* mail){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	if(!tch_mailqIsValid(mailqcb))
		return tchErrorResource;
	if(!mail)
		return tchErrorParameter;
	if(tch_port_isISR()){
		return Mempool->free(mailqcb->bpool,mail);
	}else{
		tchStatus result = tchOK;
		tch_mailq_karg karg;
		karg.chunk = mail;
		karg.timeout = 0;
		while((result = tch_port_enterSv(SV_MAILQ_FREE,(uword_t)mailqcb,(uword_t)&karg)) != tchOK){
			if(!tch_mailqIsValid(mailqcb))
				return tchErrorResource;
			switch(result){
			case tchErrorResource:
				return tchErrorResource;
			}
		}
		return result;
	}
}

tchStatus tchk_mailqFree(tch_mailqId qid,tch_mailq_karg* arg){
	if((!arg) || (!qid)) return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	if(Mempool->free(((tch_mailqCb*)qid)->bpool,arg->chunk) != tchOK)
		return tchErrorParameter;
	tchk_schedThreadResumeM((tch_thread_queue*) (&((tch_mailqCb*)qid)->wq),SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return tchOK;
}


static tchStatus tch_mailq_destroy(tch_mailqId qid){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	tchStatus result = tchOK;
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(mailqcb))
		return tchErrorResource;
	if(tch_port_isISR())
		return tchErrorISR;
	if((result = tch_port_enterSv(SV_MAILQ_DEINIT,(uword_t)qid,0)) != tchOK)
		return result;
	Mempool->destroy(mailqcb->bpool);
	MsgQ->destroy(mailqcb->msgq);
	tch_shMemFree(qid);
	return tchOK;
}

tchStatus tchk_mailqDeinit(tch_mailqId qid){
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	tchStatus result = tchOK;
	if(result == tchOK){
		tch_mailqInvalidate(qid);
		tchk_schedThreadResumeM((tch_thread_queue*) (&((tch_mailqCb*)qid)->wq) ,SCHED_THREAD_ALL,tchErrorResource,TRUE);
		return tchOK;
	}
	return tchErrorParameter;
}

static void tch_mailqValidate(tch_mailqId qid){
	((tch_mailqCb*) qid)->bstatus |= (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}

static void tch_mailqInvalidate(tch_mailqId qid){
	((tch_mailqCb*) qid)->bstatus &= ~(0xFFFF);
}

static BOOL tch_mailqIsValid(tch_mailqId qid){
	return (((tch_mailqCb*) qid)->bstatus & 0xFFFF) == (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}
