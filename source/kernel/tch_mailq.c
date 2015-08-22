/*
 * tch_mailq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */


#include "kernel/tch_err.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_ktypes.h"
#include "kernel/tch_mailq.h"

#define TCH_MAILQ_CLASS_KEY              ((uint16_t) 0x2D0D)



static tch_mailqId tch_mailqCreate(uint32_t sz,uint32_t qlen);
static void* tch_mailqAlloc(tch_mailqId qid,uint32_t timeout,tchStatus* result);
static tchStatus tch_mailqPut(tch_mailqId qid,void* mail,uint32_t timeout);
static tchEvent tch_mailqGet(tch_mailqId qid,uint32_t millisec);
static tchStatus tch_mailqFree(tch_mailqId qid,void* mail);
static tchStatus tch_mailqDestroy(tch_mailqId qid);

static inline void tch_mailqValidate(tch_mailqId qid);
static inline void tch_mailqInvalidate(tch_mailqId qid);
static inline BOOL tch_mailqIsValid(tch_mailqId qid);

__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		.create = tch_mailqCreate,
		.alloc = tch_mailqAlloc,
		.put = tch_mailqPut,
		.get = tch_mailqGet,
		.free = tch_mailqFree,
		.destroy = tch_mailqDestroy
};

const tch_mailq_ix* MailQ = &MailQStaticInstance;



DECLARE_SYSCALL_2(mailq_create,uint32_t,uint32_t,tch_mailqId);
DECLARE_SYSCALL_3(mailq_alloc,tch_mailqId,uint32_t,tchStatus*,void*);
DECLARE_SYSCALL_2(mailq_free,tch_mailqId,void*,tchStatus);
DECLARE_SYSCALL_3(mailq_put,tch_mailqId,void*,uint32_t,tchStatus);
DECLARE_SYSCALL_3(mailq_get,tch_mailqId,uint32_t,tchEvent*,tchStatus);
DECLARE_SYSCALL_1(mailq_destroy,tch_mailqId,tchStatus);



DEFINE_SYSCALL_2(mailq_create,uint32_t,sz,uint32_t,qlen,tch_mailqId){
	tch_mailqCb* mailqcb = (tch_mailqCb*) kmalloc(sizeof(tch_mailqCb));
	tch_mailqId id = tch_mailqInit(mailqcb,sz,qlen,FALSE);
	if(!id)
		KERNEL_PANIC("tch_mailq.c","can't create  mailq object");
	return id;
}

DEFINE_SYSCALL_3(mailq_alloc,tch_mailqId,qid,uint32_t,timeout,tchStatus*,result,void*){
	tchStatus lresult = tchOK;
	void* block = NULL;
	if(!result)
		result = &lresult;			// discarded
	if(!qid){
		*result = tchErrorParameter;
		return NULL;
	}
	if(!tch_mailqIsValid(qid)){
		*result = tchErrorResource;
		return NULL;
	}
	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	block = Mempool->alloc(mailq->bpool);
	if(block){
		*result = tchOK;
		return block;
	}

	tch_schedWait((tch_thread_queue*) &mailq->allocwq,timeout);
	*result = tchErrorNoMemory;
	return NULL;
}


DEFINE_SYSCALL_3(mailq_put,tch_mailqId,qid,void*,block,uint32_t,timeout,tchStatus){
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;

	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	if(mailq->qupdated >= mailq->qlen){
		if(timeout)
			tch_schedWait((tch_thread_queue*) &mailq->pwq,timeout);
		return tchErrorNoMemory;
	}
	mailq->queue[mailq->pidx++] = block;
	mailq->qupdated++;
	if(mailq->pidx >= mailq->qlen)
		mailq->pidx = 0;
	if(!cdsl_dlistIsEmpty(&mailq->gwq)){
		tchk_schedWake((tch_thread_queue* ) &mailq->gwq,1,tchInterrupted,TRUE);
	}
	return tchOK;
}

DEFINE_SYSCALL_3(mailq_get,tch_mailqId,qid,uint32_t,timeout,tchEvent*,evp,tchStatus){
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	if(mailq->qupdated <= 0){
		if(timeout)
			tch_schedWait((tch_thread_queue*) &mailq->gwq,timeout);
		evp->value.p = NULL;
		evp->status = tchErrorNoMemory;
		return evp->status;
	}
	evp->value.p = mailq->queue[mailq->gidx++];
	evp->status = tchEventMail;
	mailq->qupdated--;
	if(mailq->gidx >= mailq->qlen)
		mailq->gidx = 0;
	if(!cdsl_dlistIsEmpty(&mailq->gwq)){
		tchk_schedWake((tch_thread_queue*) &mailq->pwq,1,tchInterrupted,TRUE);
	}
	return evp->status;
}

DEFINE_SYSCALL_2(mailq_free,tch_mailqId,qid,void*,block,tchStatus){
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	if(Mempool->free(mailq->bpool,block) != tchOK)
		return tchErrorParameter;
	tchk_schedWake((tch_thread_queue*) &mailq->allocwq,SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(mailq_destroy,tch_mailqId,qid,tchStatus){

	tchStatus result = tch_mailqDeinit(qid);
	if(result != tchOK)
		return result;
	tch_mailqCb* mailq = (tch_mailqCb*) qid;

	Mempool->destroy(mailq->bpool);
	kfree(mailq->queue);
	kfree(mailq);

	return tchOK;
}

static tch_mailqId tch_mailqCreate(uint32_t sz,uint32_t qlen){
	if(!sz || !qlen)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_mailqId) __SYSCALL_2(mailq_create,sz,qlen);
}


static void* tch_mailqAlloc(tch_mailqId qid,uint32_t timeout,tchStatus* result){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	tchStatus lresult;
	void* block = NULL;
	if(!result)
		result = &lresult;
	*result = tchOK;

	if(tch_port_isISR())
		return __mailq_alloc(qid,0,result);
	do{
		block = (void*) __SYSCALL_3(mailq_alloc,qid,timeout,&result);
	}while(*result == tchInterrupted);

	return block;
}


static tchStatus tch_mailqPut(tch_mailqId qid, void* mail, uint32_t timeout){
	if(tch_port_isISR())
		return __mailq_put(qid,mail,0);
	tchStatus result;
	while((result = __SYSCALL_3(mailq_put,qid,mail,timeout)) == tchInterrupted)
		__NOP();				// prevent from optimization
	return result;
}


static tchEvent tch_mailqGet(tch_mailqId qid,uint32_t millisec){
	tchEvent evt;
	if(tch_port_isISR()){
		__mailq_get(qid,0,&evt);
		return evt;
	}
	do {
		evt.status = __SYSCALL_3(mailq_get,qid,millisec,&evt);
	}while(evt.status == tchInterrupted);
	return evt;
}

static tchStatus tch_mailqFree(tch_mailqId qid,void* mail){
	if(!mail)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __mailq_free(qid,mail);

	tchStatus result = tchOK;
	return __SYSCALL_2(mailq_free,qid,mail);
}

static tchStatus tch_mailqDestroy(tch_mailqId qid){
	if(!qid)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(mailq_destroy,qid);
}

tch_mailqId tch_mailqInit(tch_mailqCb* qcb,uint32_t sz,uint32_t qlen,BOOL isstatic){
	if(!qcb)
		return NULL;
	memset(qcb,0,sizeof(tch_mailqCb));
	qcb->queue = (void**) kmalloc(sizeof(void*) * qlen);
	qcb->bpool = Mempool->create(sz,qlen);
	qcb->__obj.__destr_fn = isstatic? (tch_kobjDestr) tch_mailqDeinit : (tch_kobjDestr) tch_mailqDestroy;

	if(!qcb->bpool || !qcb->queue){
		kfree(qcb->queue);
		Mempool->destroy(qcb->bpool);
		return NULL;
	}

	qcb->qlen = qlen;
	qcb->gidx = qcb->gidx = 0;
	qcb->__obj.__destr_fn = (tch_kobjDestr) tch_mailqDestroy;
	qcb->bsz = sz;
	cdsl_dlistInit(&qcb->gwq);
	cdsl_dlistInit(&qcb->pwq);
	cdsl_dlistInit(&qcb->allocwq);
	tch_mailqValidate(qcb);
	return (tch_mailqId) qcb;
}


tchStatus tch_mailqDeinit(tch_mailqCb* qid){
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	tch_mailqInvalidate(qid);
	tchk_schedWake((tch_thread_queue*) &mailq->pwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->gwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->allocwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	return tchOK;
}

static inline void tch_mailqValidate(tch_mailqId qid){
	((tch_mailqCb*) qid)->bstatus |= (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}

static inline void tch_mailqInvalidate(tch_mailqId qid){
	((tch_mailqCb*) qid)->bstatus &= ~(0xFFFF);
}

static inline BOOL tch_mailqIsValid(tch_mailqId qid){
	return (((tch_mailqCb*) qid)->bstatus & 0xFFFF) == (((uint32_t) qid & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
}
