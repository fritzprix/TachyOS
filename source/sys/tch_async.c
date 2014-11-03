/*
 * tch_async.c
 *
 *  Created on: 2014. 8. 8.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_async.h"
#include "tch_sys.h"

#include "tch_ltree.h"
#include "tch_btree.h"



#define TCH_ASYNC_CLASS_KEY           ((uint16_t) 0x2D0A)

#define SIG_TASK                      ((int) 0xFDA08)
#define TCH_ASYNC_DMSTK_SIZE          ((uint32_t) 1 << 11)

typedef struct tch_async_req_{
	tch_ltree_node        node;         // tree node
	tch_thread_queue      wq;
	tch_sysTask           task;         // task
	tch_threadId          requesterId;  // thread id of request
	uint32_t              timeout;      // task timeout
}tch_asyncReq;

typedef struct tch_async_nargs_t {
	int id;
	tchStatus res;
	tch_asyncReq* req;    // req
}tch_async_narg;


typedef struct tch_async_cb_t {
	uint32_t              tstatus;    // status
	tch_mailqId           tmailqId;     // mail queue for communication with kernel
	tch_ltree_node*       treqs;
}tch_async_cb;



static tch_asyncId tch_async_create(size_t tq_size);
static tchStatus tch_async_wait(tch_asyncId async,int id,tch_async_routine fn,uint32_t timeout,void* arg);
static tchStatus tch_async_notify(tch_asyncId async,int id,tchStatus res);
static tchStatus tch_async_destroy(tch_asyncId async);

static void tch_asyncValidate(tch_asyncId async);
static void tch_asyncInvalidate(tch_asyncId async);
static BOOL tch_asyncIsValid(tch_asyncId async);

static LIST_CMP_FN(tch_asyncPriorityRule);


/*                 API Structure                                             */
__attribute__((section(".data"))) static tch_async_ix ASYNC_StaticInstance = {
		tch_async_create,
		tch_async_wait,
		tch_async_notify,
		tch_async_destroy
};




const tch_async_ix* Async = &ASYNC_StaticInstance;


static tch_asyncId tch_async_create(size_t tq_size){
	tch_async_cb* async = (tch_async_cb*) Mem->alloc(sizeof(tch_async_cb)); // allocate async handle
	uStdLib->string->memset(async,0,sizeof(tch_async_cb));
	async->tmailqId = MailQ->create(sizeof(tch_asyncReq),tq_size);          // mailq to communicate to task handler thread
	async->treqs = (tch_ltree_node*) Mem->alloc(sizeof(tch_ltree_node));
	tch_ltreeInit(async->treqs,TCH_ASYNC_CLASS_KEY);
	tch_asyncValidate(async);                                               // validate async handle
	return (tch_asyncId) async;
}

static tchStatus tch_async_wait(tch_asyncId async,int id,tch_async_routine fn,uint32_t timeout,void* arg){
	tch_async_cb* cb = (tch_async_cb*) async;
	tchStatus result = osOK;
	if(!tch_asyncIsValid(async))   // check async handle validity
		return osErrorResource;
	if(tch_port_isISR())           //** wait is not allowed in ISR mode ** because which thread would wait is ambiguous
		return osErrorISR;
	tch_asyncReq* req = (tch_asyncReq*)MailQ->calloc(cb->tmailqId,timeout,&result);    // allocate request from mailbox
	if(result != osOK)                                                                 // check mailq result
		return result;

	//Initialize request
	req->requesterId = Thread->self();   // set request thread
	req->timeout = timeout;              // set timeout
	tch_ltreeInit(&req->node,id);        // initiate binary tree node (for mapping id to async req)
	tch_listInit(&req->task.tsk_nd);     // initiate list node ( for waiting req queue of systhread)
	tch_listInit((tch_lnode_t*)&req->wq);
	req->task.tsk_fn = fn;               // assign task function
	req->task.tsk_arg = arg;             // task arguement
	req->task.tsk_id = id;               // set task id as same to request async request id(BST Node Key)
	req->task.tsk_result = osOK;         // set task defaut result
	req->task.tsk_prior = Thread->getPriorty(req->requesterId);

	while((result = tch_port_enterSvFromUsr(SV_ASYNC_WAIT,(uword_t) async,(uword_t) req)) != osOK){    // system call invocation : async_wait
		if(!tch_asyncIsValid(async))                           // when thread wakes up, if async isn't valid, returns resource error
			return osErrorResource;
		switch(result){
		case osErrorResource:
			return osErrorResource;   // should return
		case osEventTimeout:          // if timeout happened
			result = tch_async_notify(async,id,osErrorTimeoutResource);   // clean up request from async binary search tree
			MailQ->free(cb->tmailqId,req);                                // free request
			return result;
		}
	}
	MailQ->free(cb->tmailqId,req);                                        // if returned result is ok
	return result;                                                        // ... free request and return
}


tchStatus tch_async_kwait(tch_asyncId async,void* async_req,void* task_queue){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_asyncReq* req = (tch_asyncReq*) async_req;
	tch_asyncReq* preq = NULL;
	if(!tch_asyncIsValid(async))      // validity check of async id
		return osErrorResource;
	tch_ltreeInsert(cb->treqs,(tch_ltree_node*) req);


	tch_thread_header* nth = NULL;
	tch_listEnqueuePriority((tch_lnode_t*) task_queue,(tch_lnode_t*) &req->task,tch_asyncPriorityRule);  // enqueue task
	if(!tch_listIsEmpty(&sysThreadPort)){
		nth = (tch_thread_header*)((tch_lnode_t*) tch_listDequeue((tch_lnode_t*) &sysThreadPort) - 1);
		tch_kernelSetResult(nth,osOK);
		tch_schedReady(nth);
	}
	tch_schedSuspend(&req->wq,req->timeout);
	return osOK;
}


static tchStatus tch_async_notify(tch_asyncId async,int id,tchStatus res){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_async_narg arg;
	tchStatus result = osOK;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	arg.res = res;
	arg.id = id;
	arg.req = NULL;
	if(tch_port_isISR()){
		tch_async_knotify(async,&arg);
		return result;
	}
	while((result = tch_port_enterSvFromUsr(SV_ASYNC_NOTIFY,(uword_t)async,(uword_t)&arg)) != osOK){
		if(!tch_asyncIsValid(async))
			return osErrorResource;
		switch(result){
		case osErrorResource:
			return osErrorResource;
		case osErrorParameter:
			return osErrorParameter;
		case osErrorTimeoutResource:
			return osErrorTimeoutResource;
		}
	}
	return result;
}

tchStatus tch_async_knotify(tch_asyncId async,void* args){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_async_narg* arg_p = (tch_async_narg*) args;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	arg_p->req = (tch_asyncReq*) tch_ltreeRemove(&cb->treqs,arg_p->id);
	if(!arg_p->req)
		return osErrorParameter;    // given id isn't correct maybe..
	if(tch_schedResumeM(&arg_p->req->wq,SCHED_THREAD_ALL,arg_p->res,TRUE))
		return osOK;
	return osErrorTimeoutResource;          // there is no waiting thread to be notified
}


static tchStatus tch_async_destroy(tch_asyncId async){
	tch_async_cb* cb = (tch_async_cb*) async;
	tchStatus result = osOK;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	if(tch_port_isISR()){
		return osErrorISR;
	}
	if((result = tch_port_enterSvFromUsr(SV_ASYNC_DESTROY,(uword_t)async,0)) != osOK)
		return result;
	Mem->free(cb->treqs);
	MailQ->destroy(cb->tmailqId);
	Mem->free(async);

	return osOK;
}


tchStatus tch_async_kdestroy(tch_asyncId async){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_asyncReq* req = NULL;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	tch_asyncInvalidate(async);
	while(!tch_ltreeIsEmpty(cb->treqs)){
		req = (tch_asyncReq*) tch_ltreeRemoveHead(&cb->treqs);
		tch_schedResumeM(&req->wq,SCHED_THREAD_ALL,osErrorResource,FALSE);
	}
	return osOK;
}


static void tch_asyncValidate(tch_asyncId async){
	((tch_async_cb*) async)->tstatus |= (((uint32_t) async & 0xFFFF) ^ TCH_ASYNC_CLASS_KEY);
}

static void tch_asyncInvalidate(tch_asyncId async){
	((tch_async_cb*) async)->tstatus &= ~(0xFFFF);
}

static BOOL tch_asyncIsValid(tch_asyncId async){
	return (((tch_async_cb*) async)->tstatus & 0xFFFF) == (((uint32_t) async & 0xFFFF) ^ TCH_ASYNC_CLASS_KEY);
}

static LIST_CMP_FN(tch_asyncPriorityRule){
	return ((tch_sysTask*) prior)->tsk_prior > ((tch_sysTask*) post)->tsk_prior;
}

