/*
 * tch_ptask.c
 *
 *  Created on: 2014. 11. 5.
 *      Author: innocentevil
 */

#include "tch_kernel.h"
#include "tch_ptask.h"
#include "tch_ltree.h"


#define TCH_PTASK_QSZ          ((uint32_t) 10)
#define TCH_PTASK_CLASS_KEY    ((uint16_t) 0x91AD)

typedef struct tch_ptask_t tch_ptask;


struct tch_ptask_t {
	tch_ptask_fn           fn;
	tch_ptask_prior        prior;
	void*                  arg;
	uint8_t                status;
	int                    id;
}__attribute__((packed));




static tchStatus tch_ptask_schedule(int id, tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout);
static void tch_ptaskValidate(tch_ptask* task);
static void tch_ptaskInvalidate(tch_ptask* task);
static BOOL tch_ptaskIsValid(tch_ptask* task);
static DECLARE_THREADROUTINE(tch_ptaskHandleLoop);

static tch_ptask_ix PTASK_StaticInstance;
static DECLARE_THREADSTACK(tch_ptask_stk,1 << 10);
static tch_threadId tch_ptaskThread;
static tch_mailqId tch_ptaskMq;


const tch_ptask_ix* tch_initpTask(const tch* env){

	tch_ptaskMq = NULL;

	tch_threadCfg thcfg;
	thcfg._t_name = "ptask_looper";
	thcfg._t_routine = tch_ptaskHandleLoop;
	thcfg._t_stack = tch_ptask_stk;
	thcfg.t_stackSize = (1 << 10);
	thcfg.t_proior = KThread;

	tch_ptaskThread = env->Thread->create(&thcfg,NULL);
	tch_schedReady(tch_ptaskThread);


	PTASK_StaticInstance.schedule = tch_ptask_schedule;
	return &PTASK_StaticInstance;
}

static tchStatus tch_ptask_schedule(int id,tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout){

	tchStatus result = osOK;
	struct tch_ptask_t* task = MailQ->calloc(tch_ptaskMq,timeout,&result);
	task->arg = arg;
	task->fn = rn;
	task->id = id;
	task->prior = prior;
	tch_ptaskValidate(task);
	result = MailQ->put(tch_ptaskMq,task);

	return result;
}

static void tch_ptaskValidate(tch_ptask* task){
	task->status |= (((uint32_t) task ^ TCH_PTASK_CLASS_KEY) & 0xFFFF);
}

static void tch_ptaskInvalidate(tch_ptask* task){
	task->status &= ~0xFFFF;
}

static BOOL tch_ptaskIsValid(tch_ptask* task){
	return (task->status & 0xFFFF) == (((uint32_t) task ^ TCH_PTASK_CLASS_KEY) & 0xFFFF);
}

static DECLARE_THREADROUTINE(tch_ptaskHandleLoop){

	if(tch_kernel_initCrt0(env) != osOK)
		tch_kernel_errorHandler(FALSE,osErrorOS);

	tch_ptask* task = NULL;
	if(!tch_ptaskMq)
		tch_ptaskMq = env->MailQ->create(sizeof(tch_ptask),TCH_PTASK_QSZ);
	osEvent evt;
	env->uStdLib->string->memset(&evt,0,sizeof(osEvent));
	while(TRUE){
		evt = env->MailQ->get(tch_ptaskMq,osWaitForever);
		if(evt.status == osEventMail){
			if(tch_ptaskIsValid(task)){
				task = (tch_ptask*) evt.value.p;
				task->fn(task->id,env,task->arg);
				tch_ptaskInvalidate(task);
				env->MailQ->free(tch_ptaskMq,task);
			}else{
				return osErrorOS;
			}
		}
	}
	return osOK;
}

