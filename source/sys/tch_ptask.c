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
	uint16_t               status;
	int                    id;
}__attribute__((packed));




static tchStatus tch_ptask_schedule(int id, tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout);
static void tch_ptaskValidate(tch_ptask* task);
static void tch_ptaskInvalidate(tch_ptask* task);
static BOOL tch_ptaskIsValid(tch_ptask* task);

__attribute__((section(".data")))static tch_ptask_ix PTASK_StaticInstance = {
		tch_ptask_schedule
};



static tchStatus tch_ptask_schedule(int id,tch_ptask_fn rn,void* arg,tch_ptask_prior prior,uint32_t timeout){

	tchStatus result = osOK;
	struct tch_ptask_t* task = MailQ->calloc(sysTaskQ,timeout,&result);
	task->arg = arg;
	task->fn = rn;
	task->id = id;
	task->prior = prior;
	tch_ptaskValidate(task);
	result = MailQ->put(sysTaskQ,task);

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

