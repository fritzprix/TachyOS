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

typedef struct tch_mailqCb {
	tch_kobj     	 	__obj;
	uint32_t      	 	bstatus;         // state
	uint32_t      	 	bsz;
	uint32_t	  	 	qlen;
	uint32_t	 	 	pidx;
	uint32_t	 	 	gidx;
	uint32_t			qupdated;
	cdsl_dlistNode_t 	pwq;
	cdsl_dlistNode_t 	gwq;
	void**			queue;
	tch_mpoolId   		bpool;
	cdsl_dlistNode_t  	allocwq;
}tch_mailqCb;


static tch_mailqId tch_mailq_create(uint32_t sz,uint32_t qlen);
static void* tch_mailq_alloc(tch_mailqId qid,uint32_t timeout,tchStatus* result);
static tchStatus tch_mailq_put(tch_mailqId qid,void* mail,uint32_t timeout);
static tchEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec);
static tchStatus tch_mailq_free(tch_mailqId qid,void* mail);
static tchStatus tch_mailq_destroy(tch_mailqId qid);

static inline void tch_mailqValidate(tch_mailqId qid);
static inline void tch_mailqInvalidate(tch_mailqId qid);
static inline BOOL tch_mailqIsValid(tch_mailqId qid);

__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		.create = tch_mailq_create,
		.alloc = tch_mailq_alloc,
		.put = tch_mailq_put,
		.get = tch_mailq_get,
		.free = tch_mailq_free,
		.destroy = tch_mailq_destroy
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
	if(!mailqcb)
		KERNEL_PANIC("tch_mailq.c","can't create mailq object");
	memset(mailqcb,0,sizeof(tch_mailqCb));
	mailqcb->queue = kmalloc(sizeof(void*) * qlen);
	mailqcb->bpool = Mempool->create(sz,qlen);
	if(!mailqcb->bpool || !mailqcb->queue)
		KERNEL_PANIC("tch_mailq.c","can't created mailq object");
	mailqcb->qlen = qlen;
	mailqcb->gidx = mailqcb->gidx = 0;
	mailqcb->__obj.__destr_fn = (tch_kobjDestr) tch_mailq_destroy;
	mailqcb->bsz = sz;
	cdsl_dlistInit(&mailqcb->gwq);
	cdsl_dlistInit(&mailqcb->pwq);
	cdsl_dlistInit(&mailqcb->allocwq);
	tch_mailqValidate(mailqcb);
	return mailqcb;
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

	tchk_schedWait((tch_thread_queue*) &mailq->allocwq,timeout);
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
			tchk_schedWait(&mailq->pwq,timeout);
		return tchErrorNoMemory;
	}
	mailq->queue[mailq->pidx++] = block;
	mailq->qupdated++;
	if(mailq->pidx >= mailq->qlen)
		mailq->pidx = 0;
	if(!cdsl_dlistIsEmpty(&mailq->gwq)){
		tchk_schedWake(&mailq->gwq,1,tchInterrupted,TRUE);
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
			tchk_schedWait(&mailq->gwq,timeout);
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
		tchk_schedWake(&mailq->pwq,1,tchInterrupted,TRUE);
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
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;

	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	tch_mailqInvalidate(qid);
	tchk_schedWake((tch_thread_queue*) &mailq->pwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->gwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->allocwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);

	Mempool->destroy(mailq->bpool);
	kfree(mailq->queue);
	kfree(mailq);

	return tchOK;
}

static tch_mailqId tch_mailq_create(uint32_t sz,uint32_t qlen){
	if(!sz || !qlen)
		return NULL;
	if(tch_port_isISR())
		return NULL;
	return (tch_mailqId) __SYSCALL_2(mailq_create,sz,qlen);
}


static void* tch_mailq_alloc(tch_mailqId qid,uint32_t timeout,tchStatus* result){
	tch_mailqCb* mailqcb = (tch_mailqCb*) qid;
	tchStatus lresult;
	void* block = NULL;
	if(!result)
		result = &lresult;
	*result = tchOK;

	if(tch_port_isISR())
		return __mailq_alloc(qid,0,result);
	do{
		block = __SYSCALL_3(mailq_alloc,qid,timeout,&result);
	}while(*result == tchInterrupted);

	return block;
}


static tchStatus tch_mailq_put(tch_mailqId qid, void* mail, uint32_t timeout){
	if(tch_port_isISR())
		return __mailq_put(qid,mail,0);
	tchStatus result;
	while((result = __SYSCALL_3(mailq_put,qid,mail,timeout)) == tchInterrupted)
		__NOP();				// prevent from optimization
	return result;
}


static tchEvent tch_mailq_get(tch_mailqId qid,uint32_t millisec){
	tchEvent evt;
	if(tch_port_isISR()){
		__mailq_get(qid,0,&evt);
		return evt;
	}
	do {
		__SYSCALL_3(mailq_get,qid,millisec,&evt);
	}while(evt.status == tchInterrupted);
	return evt;
}

static tchStatus tch_mailq_free(tch_mailqId qid,void* mail){
	if(!mail)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __mailq_free(qid,mail);

	tchStatus result = tchOK;
	tch_mailq_karg karg;
	karg.chunk = mail;
	karg.timeout = 0;
	return __SYSCALL_2(mailq_free,qid,mail);
}

tchStatus tchk_mailqFree(tch_mailqId qid,tch_mailq_karg* arg){
	if((!arg) || (!qid)) return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	if(Mempool->free(((tch_mailqCb*)qid)->bpool,arg->chunk) != tchOK)
		return tchErrorParameter;
	tchk_schedWake((tch_thread_queue*) (&((tch_mailqCb*)qid)->allocwq),SCHED_THREAD_ALL,tchErrorNoMemory,TRUE);
	return tchOK;
}

static tchStatus tch_mailq_destroy(tch_mailqId qid){
	if(!qid)
		return tchErrorParameter;
	if(tch_port_isISR())
		return tchErrorISR;
	return __SYSCALL_1(mailq_destroy,qid);
}

tchStatus tchk_mailqDeinit(tch_mailqId qid){
	if(!qid)
		return tchErrorParameter;
	if(!tch_mailqIsValid(qid))
		return tchErrorResource;
	tch_mailqCb* mailq = (tch_mailqCb*) qid;
	tch_mailqInvalidate(qid);
	tchk_schedWake((tch_thread_queue*) &mailq->pwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->gwq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	tchk_schedWake((tch_thread_queue*) &mailq->allocwq,SCHED_THREAD_ALL,tchErrorResource,TRUE);

	Mempool->destroy(mailq->bpool);
	kfree(mailq->queue);
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
