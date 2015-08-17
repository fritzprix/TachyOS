/*
 * tch_sem.c
 *
 *  Created on: 2014. 8. 19.
 *      Author: innocentevil
 */


#include "kernel/tch_ktypes.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_err.h"
#include "cdsl_dlist.h"


#define TCH_SEMAPHORE_CLASS_KEY                      ((uint16_t) 0x1A0A)


typedef struct _tch_sem_t {
	tch_kobj          	__obj;
	uint32_t        	state;
	uint32_t     	    count;
	cdsl_dlistNode_t    wq;
} tch_semaphore_cb;

DECLARE_SYSCALL_1(semaphore_create,uint32_t, tch_semId);
DECLARE_SYSCALL_2(semaphore_lock,tch_semId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(semaphore_unlock,tch_semId,tchStatus);
DECLARE_SYSCALL_1(semaphore_destroy,tch_semId,tchStatus);


static tch_semId tch_semaphore_create(uint32_t count);
static tchStatus tch_semaphore_lock(tch_semId id,uint32_t timeout);
static tchStatus tch_semaphore_unlock(tch_semId id);
static tchStatus tch_semaphore_destroy(tch_semId id);


static void tch_semaphoreValidate(tch_semId sid);
static void tch_semaphoreInvalidate(tch_semId sid);
static BOOL tch_semaphoreIsValid(tch_semId sid);





__attribute__((section(".data"))) static tch_semaph_ix Semaphore_StaticInstance = {
		.create = tch_semaphore_create,
		.lock = tch_semaphore_lock,
		.unlock = tch_semaphore_unlock,
		.destroy = tch_semaphore_destroy
};

const tch_semaph_ix* Sem = (const tch_semaph_ix*) &Semaphore_StaticInstance;

DEFINE_SYSCALL_1(semaphore_create,uint32_t,count,tch_semId){
	tch_semaphore_cb* semCb = (tch_semaphore_cb*) kmalloc(sizeof(tch_semaphore_cb));
	if(!semCb)

	semCb->__obj.__destr_fn = (tch_kobjDestr) tch_semaphore_destroy;
	semCb->count = count;
	cdsl_dlistInit(&semCb->wq);
	tch_semaphoreValidate(semCb);
	return semCb;
}


DEFINE_SYSCALL_2(semaphore_lock,tch_semId, semid,uint32_t,timeout,tchStatus){
	if(!semid)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(semid))
		return tchErrorResource;

	tch_semaphore_cb* sem = (tch_semaphore_cb*) semid;
	tchStatus result = tchOK;
	if(!tch_port_exclusiveCompareDecrement((int*)&sem->count,0)){
		if(!timeout)
			return tchErrorResource;
		tchk_schedWait((tch_thread_queue*) &sem->wq,timeout);
	}
	return tchOK;
}


DEFINE_SYSCALL_1(semaphore_unlock,tch_semId,semid,tchStatus){
	if(!semid)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(semid))
		return tchErrorResource;
	tch_semaphore_cb* sem = (tch_semaphore_cb*) semid;
	sem->count++;
	if(!cdsl_dlistIsEmpty(&sem->wq)){
		tchk_schedWake((tch_thread_queue*) &sem->wq, SCHED_THREAD_ALL,tchInterrupted,TRUE);
	}
	return tchOK;
}

DEFINE_SYSCALL_1(semaphore_destroy,tch_semId,semid,tchStatus){
	if(!semid)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(semid))
		return tchErrorResource;
	tch_semaphore_cb* sem = (tch_semaphore_cb*) semid;
	sem->state = 0;
	sem->count = 0;
	tch_semaphoreInvalidate(sem);
	if(!cdsl_dlistIsEmpty(&sem->wq)){
		tchk_schedWake((tch_thread_queue*) &sem->wq,SCHED_THREAD_ALL,tchErrorResource,TRUE);
	}
	return tchOK;
}


static tch_semId tch_semaphore_create(uint32_t count){
	if(!count)
		return NULL;		// count value should be larger than zero
	if(tch_port_isISR())
		return NULL;		// semaphore can't be created isr context
	return (tch_semId) __SYSCALL_1(semaphore_create,count);
}

static tchStatus tch_semaphore_lock(tch_semId id,uint32_t timeout){
	if(!id)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __semaphore_lock(id,0);
	tchStatus result = tchOK;
	while((result = __SYSCALL_2(semaphore_lock,id,timeout)) == tchInterrupted);
	if(result == tchEventTimeout)
		return tchErrorTimeoutResource;
	return result;
 }

static tchStatus tch_semaphore_unlock(tch_semId id){
	if(!tch_semaphoreIsValid(id))
		return tchErrorResource;
	if(tch_port_isISR())
		return __semaphore_unlock(id);
	return __SYSCALL_1(semaphore_unlock,id);
}

static tchStatus tch_semaphore_destroy(tch_semId id){
	if(!tch_semaphoreIsValid(id))
		return tchErrorParameter;
	tch_semaphore_cb* sem = (tch_semaphore_cb*) id;
	sem->state = 0;
	sem->count = 0;
	tch_semaphoreInvalidate(id);
	if(!cdsl_dlistIsEmpty(&sem->wq)){
		if(tch_port_isISR())
			tchk_schedWake((tch_thread_queue*) &sem->wq,SCHED_THREAD_ALL,tchOK,TRUE);
		else
			tch_port_enterSv(SV_THREAD_RESUMEALL,(uint32_t)&sem->wq,tchErrorResource,0);
	}
	tch_shmFree(sem);
	return  tchOK;
}


static void tch_semaphoreValidate(tch_semId sid){
	((tch_semaphore_cb*) sid)->state |= (((uint32_t) sid & 0xFFFF) ^ TCH_SEMAPHORE_CLASS_KEY);
}

static void tch_semaphoreInvalidate(tch_semId sid){
	((tch_semaphore_cb*) sid)->state &= ~(0xFFFF);
}

static BOOL tch_semaphoreIsValid(tch_semId sid){
	return (((tch_semaphore_cb*) sid)->state & 0xFFFF) == (((uint32_t) sid & 0xFFFF) ^ TCH_SEMAPHORE_CLASS_KEY);
}
