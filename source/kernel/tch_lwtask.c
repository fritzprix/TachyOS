/*
 * tch_lwtask.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_lwtask.h"
#include "cdsl_rbtree.h"

#define LWSTATUS_PENDING		((uint8_t) 1)
#define LWSTATUS_DONE			((uint8_t) 2)
#define LWSTATUS_RUN			((uint8_t) 4)

struct lw_task {
	rb_treeNode_t		rbn;
	lwtask_routine 		do_job;
	uint8_t				prior;
	void*				arg;
	uint8_t 			status;
	tch_mtxId			lock;
	tch_condvId			condv;
	cdsl_dlistNode_t	tsk_qn;
};

static DECLARE_COMPARE_FN(lwtask_priority_rule);

static tch_barId		looper_waitq;
static cdsl_dlistNode_t	tsk_queue;
static rb_treeNode_t*	tsk_root;
static uint32_t			tsk_cnt;


static tch_mtxId		tsk_lock;
static tch_condvId		tsk_condv;

void __lwtsk_start_loop(){

	looper_waitq = tch_rti->Barrier->create();
	tsk_root = NULL;
	tsk_cnt = 0;
	cdsl_dlistInit(&tsk_queue);
	tsk_lock = tch_rti->Mtx->create();
	tsk_condv = tch_rti->Condv->create();

	struct lw_task* tsk = NULL;
	while(TRUE){
		do {
			tch_port_atomicBegin();
			tsk = (struct lw_task*) cdsl_dlistDequeue(&tsk_queue);
			tch_port_atomicEnd();

			if (!tsk) {
				if (tch_rti->Barrier->wait(looper_waitq, tchWaitForever) != tchOK) {
					return;	//break loop and return (means system termination)
				}
			}
		} while (!tsk);

		tsk = container_of(tsk,struct lw_task,tsk_qn);
		tch_rti->Mtx->lock(tsk->lock,tchWaitForever);
		if (tsk->status != LWSTATUS_DONE) {
			tsk->status = LWSTATUS_RUN;
			tsk->do_job(tsk->rbn.key, tch_rti, tsk->arg);// dead lock alert!! (any invocation of lwtask function causes dead lock
														 // note : task struct is locked at every lwtask interface
			tsk->status = LWSTATUS_DONE;
			tch_rti->Condv->wakeAll(tsk->condv);
		}
		tch_rti->Mtx->unlock(tsk->lock);
	}

}



int tch_lwtsk_registerTask(lwtask_routine fnp,uint8_t prior){
	if(!fnp)
		return -1;
	struct lw_task* tsk = (struct lw_task*) kmalloc(sizeof(struct lw_task));
	tsk->condv = tch_rti->Condv->create();
	tsk->lock = tch_rti->Mtx->create();
	tsk->arg = NULL;
	tsk->do_job = fnp;
	tsk->prior = prior;
	tsk->status = LWSTATUS_DONE;
	cdsl_dlistInit(&tsk->tsk_qn);
	cdsl_rbtreeNodeInit(&tsk->rbn,tsk_cnt++);

	tch_port_atomicBegin();
	cdsl_rbtreeInsert(&tsk_root,&tsk->rbn);
	tch_port_atomicEnd();

	return tsk->rbn.key;
}

void tch_lwtsk_unregisterTask(int tsk_id){
	if(tsk_id < 0)
		return;
	struct lw_task* tsk = (struct lw_task*) cdsl_rbtreeDelete(&tsk_root,tsk_id);
	if(!tsk)
		return;
	tsk = container_of(tsk,struct lw_task,rbn);
	if(tch_rti->Mtx->lock(tsk->lock,tchWaitForever) != tchOK)
		return;		// may be this task is already destroyed or invalid
	while(tsk->status != LWSTATUS_DONE){			//wait until done
		if(tch_rti->Condv->wait(tsk->condv,tsk->lock,tchWaitForever) != tchOK)
			return;
	}
	tch_rti->Mtx->unlock(tsk->lock);


	tch_rti->Mtx->destroy(tsk->lock);
	tch_rti->Condv->destroy(tsk->condv);
	tchk_kernelHeapFree(tsk);

}


void tch_lwtsk_request(int tsk_id,void* arg, BOOL canblock){
	if((tsk_id > tsk_cnt) || (tsk_id < 0))
		return;

	struct lw_task* tsk = (struct lw_task*) cdsl_rbtreeLookup(&tsk_root,tsk_id);
	if(!tsk)
		return;

	tsk = container_of(tsk,struct lw_task,rbn);
	if(tch_port_isISR()){
		canblock = FALSE;
	}

	if (canblock) {
		if (tch_rti->Mtx->lock(tsk->lock, tchWaitForever) != tchOK)
			return;
		while (tsk->status != LWSTATUS_DONE) {
			tch_rti->Condv->wait(tsk->condv, tsk->lock, tchWaitForever);
		}
		tsk->status = LWSTATUS_PENDING;
		tsk->arg = arg;
		tch_rti->Mtx->unlock(tsk->lock);
	}else{
		tch_port_atomicBegin();
		if(tsk->status != LWSTATUS_DONE){
			tch_port_atomicEnd();
			return;
		}

		tsk->status = LWSTATUS_PENDING;
		tsk->arg = arg;
		tch_port_atomicEnd();
	}

	tch_port_atomicBegin();
	cdsl_dlistEnqueuePriority(&tsk_queue,&tsk->tsk_qn,lwtask_priority_rule);
	tch_port_atomicEnd();

	tch_rti->Barrier->signal(looper_waitq,tchOK);


	if(canblock){
		if (tch_rti->Mtx->lock(tsk->lock, tchWaitForever) != tchOK)
			return;
		while (tsk->status != LWSTATUS_DONE) {
			tch_rti->Condv->wait(tsk->condv, tsk->lock, tchWaitForever);
		}
		tch_rti->Mtx->unlock(tsk->lock);
	}
}

void tch_lwtsk_cancel(int tsk_id){
	if((tsk_id > tsk_cnt) || (tsk_id < 0))
		return;

	struct lw_task* tsk = (struct lw_task*) cdsl_rbtreeLookup(&tsk_root,tsk_id);
	if(!tsk)
		return;

	tsk = container_of(tsk,struct lw_task,rbn);
	if(tch_rti->Mtx->lock(tsk->lock,tchWaitForever) != tchOK)
		return;
	tsk->status = LWSTATUS_DONE;
	tch_port_atomicBegin();
	cdsl_dlistRemove(&tsk->tsk_qn);
	tch_port_atomicEnd();
	tch_rti->Mtx->unlock(tsk->lock);
}

static DECLARE_COMPARE_FN(lwtask_priority_rule){
	return ((struct lw_task*)container_of(a,struct lw_task,tsk_qn))->prior > ((struct lw_task*)container_of(b,struct lw_task,tsk_qn))->prior? a : b;
}

