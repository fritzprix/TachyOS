/*
 * tch_mailq.c
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_mailq.h"

#define TCH_MAILQ_CLASS_KEY              ((uint16_t) 0x2D0D)
#define tch_mailqValidate(mailq)         ((tch_mailq_cb*) mailq)->bstate = (((uint32_t) mailq & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY);
#define tch_mailqInvalidate(mailq)       ((tch_mailq_cb*) mailq)->bstate &= ~(0xFFFF)
#define tch_mailqIsValid(mailq)          ((((tch_mailq_cb*) mailq)->bstate & 0xFFFF) == (((uint32_t) mailq & 0xFFFF) ^ TCH_MAILQ_CLASS_KEY))



static tch_mailQue_id tch_mailq_create(size_t sz,uint32_t qlen);
static void* tch_mailq_alloc(tch_mailQue_id qid,uint32_t millisec);
static void* tch_mailq_calloc(tch_mailQue_id qid,uint32_t millisec);
static tchStatus tch_mailq_put(tch_mailQue_id qid,void* mail);
static osEvent tch_mailq_get(tch_mailQue_id qid,uint32_t millisec);
static tchStatus tch_mailq_free(tch_mailQue_id qid,void* mail);


__attribute__((section(".data"))) static tch_mailq_ix MailQStaticInstance = {
		tch_mailq_create,
		tch_mailq_alloc,
		tch_mailq_calloc,
		tch_mailq_put,
		tch_mailq_get,
		tch_mailq_free
};

const tch_mailq_ix* MailQ = &MailQStaticInstance;

typedef struct _tch_mailq_cb {
	uint32_t      bstate;
	void*         bp;
	size_t        bsz;
	size_t        updated;
	size_t        pidx;
	size_t        gidx;
	tch_mpoolId   bpool;
	tch_mtxId     bMtx;
	tch_condvId   bAllocV;
}tch_mailq_cb;


static tch_mailQue_id tch_mailq_create(size_t sz,uint32_t qlen){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) Mem->alloc(sizeof(tch_mailq_cb) + sizeof(void*) * qlen);
	mailqcb->bp = ((tch_mailq_cb*) mailqcb + 1);
	mailqcb->bAllocV = Condv->create();
	mailqcb->bMtx = Mtx->create();
	mailqcb->bsz = qlen;
	mailqcb->bpool = Mempool->create(sz,qlen);

	tch_mailqValidate(mailqcb);
	return (tch_mailQue_id) mailqcb;
}

static void* tch_mailq_alloc(tch_mailQue_id qid,uint32_t millisec){
	tch_mailq_cb* mailqcb = (tch_mailq_cb*) qid;
	if(!tch_mailqIsValid(mailqcb))
		return NULL;
	if(tch_port_isISR()){
	}else{

	}
}

static void* tch_mailq_calloc(tch_mailQue_id qid,uint32_t millisec){

}

static tchStatus tch_mailq_put(tch_mailQue_id qid,void* mail){

}

static osEvent tch_mailq_get(tch_mailQue_id qid,uint32_t millisec){

}

static tchStatus tch_mailq_free(tch_mailQue_id qid,void* mail){

}
