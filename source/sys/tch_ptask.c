/*
 * tch_ptask.c
 *
 *  Created on: 2014. 11. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_ptask.h"


#define TCH_PTASK_QSZ          ((uint32_t) 10)

struct tch_ptask_t {
	tch_ltree_node         ln;
	tch_ptask_fn           fn;
	tch_ptask_prior        prior;
	void*                  arg;
	uint8_t                status;
	int                    id;
}__attribute__((packed));


static tchStatus tch_ptask_schedule(tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout);
static DECLARE_THREADROUTINE(tch_ptaskHandleLoop);

static tch_ptask_ix PTASK_StaticInstance;
static DECLARE_THREADSTACK(tch_ptask_stk,1 << 9);
static tch_threadId tch_ptaskThread;
static tch_mailqId tch_ptaskMq;

const tch_ptask_ix* tch_initpTask(const tch* env){

	tch_ptaskMq = NULL;

	tch_threadCfg thcfg;
	thcfg._t_name = "ptask_looper";
	thcfg._t_routine = tch_ptaskHandleLoop;
	thcfg._t_stack = tch_ptask_stk;
	thcfg.t_stackSize = (1 << 9);
	thcfg.t_proior = KThread;

	tch_ptaskThread = env->Thread->create();
	tch_schedReady(tch_ptaskThread);


	PTASK_StaticInstance.schedule = tch_ptask_schedule;
	return &PTASK_StaticInstance;
}

static tchStatus tch_ptask_schedule(tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout){

}

static DECLARE_THREADROUTINE(tch_ptaskHandleLoop){

	struct tch_ptask_t* task = NULL;
	if(!tch_ptaskMq)
		tch_ptaskMq = env->MailQ->create(sizeof(struct tch_ptask_t),TCH_PTASK_QSZ);
	osEvent evt;
	env->uStdLib->string->memset(&evt,0,sizeof(osEvent));
	while(TRUE){
		evt = env->MailQ->get(tch_ptaskMq,osWaitForever);
		if(evt.status == osEventMail){
			task = (struct tch_ptask_t*) evt.value.p;
			task->fn(task->id,env,task->arg);
		}
	}
}

