/*
 * tch_async.c
 *
 *  Created on: 2014. 8. 8.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_async.h"
#include "tch_sys.h"

#include "tch_btree.h"



#define TCH_ASYNC_PRIOR_VERYHIGH      ((uint8_t) 5)
#define TCH_ASYNC_PRIOR_HIGH          ((uint8_t) 4)
#define TCH_ASYNC_PRIOR_NORMAL        ((uint8_t) 3)
#define TCH_ASYNC_PRIOR_LOW           ((uint8_t) 2)
#define TCH_ASYNC_PRIOR_VERYLOW       ((uint8_t) 1)

#define TCH_ASYNC_CLASS_KEY           ((uint16_t) 0x2D0A)
#define tch_asyncValidate(async)      ((tch_async_cb*) async)->tstatus = (((uint32_t) async & 0xFFFF) ^ TCH_ASYNC_CLASS_KEY)
#define tch_asyncInvalidate(async)    ((tch_async_cb*) async)->tstatus &= ~(0xFFFF)
#define tch_asyncIsValid(async)       (((tch_async_cb*) async)->tstatus & 0xFFFF) ==  (((uint32_t) async & 0xFFFF) ^ TCH_ASYNC_CLASS_KEY)

#define SIG_TASK                      ((int) 0xFDA08)
#define TCH_ASYNC_DMSTK_SIZE          ((uint32_t) 1 << 11)




typedef struct tch_async_req_{
	tch_btree_node        node;         // tree node
	tch_sysTask           task;
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
	tch_thread_queue      twq;        // waiting thread queue
	tch_btree_node*       tmap;       // task ID - tch_asyncTask map
}tch_async_cb;






static tch_asyncId tch_async_create(size_t tq_size);
static tchStatus tch_async_wait(tch_asyncId async,int id,tch_async_routine fn,uint32_t timeout,void* arg);
static tchStatus tch_async_notify(tch_asyncId async,int id,tchStatus res);
static tchStatus tch_async_destroy(tch_asyncId async);

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
	tch_async_cb* async = (tch_async_cb*) Mem->alloc(sizeof(tch_async_cb));
	async->tmailqId = MailQ->create(sizeof(tch_asyncReq),tq_size);
	async->tmap = (tch_btree_node*) Mem->alloc(sizeof(tch_btree_node));
	tch_listInit(&async->twq);
	tch_btreeInit(async->tmap,TCH_ASYNC_CLASS_KEY);
	tch_asyncValidate(async);
	return (tch_asyncId) async;
}

static tchStatus tch_async_wait(tch_asyncId async,int id,tch_async_routine fn,uint32_t timeout,void* arg){
	tch_async_cb* cb = (tch_async_cb*) async;
	tchStatus result = osOK;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	tch_asyncReq* req = (tch_asyncReq*)MailQ->calloc(cb->tmailqId,timeout,&result);
	if(result != osOK)   // check mailq result
		return result;
	req->requesterId = Thread->self();
	req->timeout = timeout;
	tch_btreeInit(&req->node,id);    // initiate binary tree node (for mapping id to async req)
	tch_listInit(&req->task.tsk_nd); // initiate list node ( for waiting req queue of systhread)
	req->task.tsk_fn = fn;
	req->task.tsk_arg = arg;
	req->task.tsk_id = id;           // set task id as same to request async request id(BST Node Key)
	req->task.tsk_result = osOK;
	req->task.tsk_prior = Thread->getPriorty(req->requesterId);

	while((result = tch_port_enterSvFromUsr(SV_ASYNC_WAIT,async,req)) != osOK){
		if(!tch_asyncIsValid(async))
			return osErrorResource;
		switch(result){
		case osErrorResource:
			return osErrorResource;
		case osEventTimeout:
			tch_btree_delete(&cb->tmap,id);           //should remove node in the tree
			return osErrorTimeoutResource;
		}
	}
	return result;
}

tchStatus tch_async_kwait(tch_asyncId async,void* async_req,void* task_queue){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_asyncReq* req = (tch_asyncReq*) async_req;
	if(!tch_asyncIsValid(async)) // validity check of async id
		return osErrorResource;
	tch_btree_insert(cb->tmap,req);    // insert request into binary search tree of async control block for fast lookup in notification
	tch_listEnqueuePriority((tch_lnode_t*) task_queue,&req->task,tch_asyncPriorityRule);   // enqueue task in sys thread task Q

	tch_schedSuspend(&cb->twq,req->timeout);   // suspend requester
	if(!tch_listIsEmpty(&sysThreadPort))
		//tch_schedResumeAll(&sysThreadPort,osOK,FALSE);
		tch_schedResumeM(&sysThreadPort,SCHED_THREAD_ALL,osOK,FALSE);
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
//		tch_port_enterSvFromIsr(SV_ASYNC_NOTIFY,async,&arg);
		tch_port_kernel_lock();
		tch_async_knotify(async,&arg);
		tch_port_kernel_unlock();
		MailQ->free(cb->tmailqId,arg.req);
		return result;
	}
	while((result = tch_port_enterSvFromUsr(SV_ASYNC_NOTIFY,async,&arg)) != osOK){
		if(!tch_asyncIsValid(async))
			return osErrorResource;
		switch(result){
		case osErrorResource:
			return osErrorResource;
		}
	}
	MailQ->free(cb->tmailqId,arg.req);
	return result;
}

tchStatus tch_async_knotify(tch_asyncId async,void* args){
	tch_async_cb* cb = (tch_async_cb*) async;
	tch_async_narg* arg_p = (tch_async_narg*) args;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	arg_p->req = tch_btree_delete(&cb->tmap,arg_p->id);
	if(!arg_p->req)
		return osErrorParameter;    // given id isn't correct maybe..
	arg_p->req->task.tsk_result = arg_p->res;
	if(tch_schedResumeThread(&cb->twq,arg_p->req->requesterId,osOK,FALSE))
		return osOK;
	return osErrorOS;          // there is no waiting thread to be notified
}

static tchStatus tch_async_destroy(tch_asyncId async){
	tch_async_cb* cb = (tch_async_cb*) async;
	tchStatus result = osOK;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	if(tch_port_isISR()){
		return osErrorISR;
	}
	if((result = tch_port_enterSvFromUsr(SV_ASYNC_DESTROY,async,0)) != osOK)
		return result;
	Mem->free(cb->tmap);
	if((result = MailQ->destroy(cb->tmailqId)) != osOK)
		return result;
	return osOK;
}

tchStatus tch_async_kdestroy(tch_asyncId async){
	tch_async_cb* cb = (tch_async_cb*) async;
	if(!tch_asyncIsValid(async))
		return osErrorResource;
	tch_asyncInvalidate(async);
	tch_schedResumeM(&cb->twq,1,osErrorResource,TRUE);
	return osOK;
}


static LIST_CMP_FN(tch_asyncPriorityRule){
	return ((tch_sysTask*) prior)->tsk_prior > ((tch_sysTask*) post)->tsk_prior;
}



/*
 * #define SV_ASYNC_START                   ((uint32_t) 0x2A)               ///< Supervisor call id to start async task
#define SV_ASYNC_BLSTART                 ((uint32_t) 0x2B)               ///< Supervisor call id to start async task with blocking current execution
#define SV_ASYNC_NOTIFY                  ((uint32_t) 0x2C)
 */



/*

static tch_asyncId tch_async_create(tch_async_routine fn,uint8_t prior){
	tch_async_cb* asyn_cb = (tch_async_cb*) Mem->alloc(sizeof(tch_async_cb));
	asyn_cb->async_fn = fn;
	asyn_cb->prior = prior;
	tch_btreeInit(&asyn_cb->tcmplq,asyn_cb);
	tch_listInit(&asyn_cb->twaitq);
	return (tch_asyncId) asyn_cb;
}
*/






/*
// initiailize thread local variables
	tch_async_svthread_cb* thcb = (tch_async_svthread_cb*) sys->Thread->getArg();
	tchStatus result = osOK;
	while(TRUE){
		while((result = tch_port_enterSvFromUsr(0,0,0)) == osOK){
		}
		sys->Sig->wait(SIG_TASK,osWaitForever);    // wait task
	}
}
*/


