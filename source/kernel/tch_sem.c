/*
 * tch_sem.c
 *
 *  Created on: 2014. 8. 19.
 *      Author: innocentevil
 */


#include "kernel/tch_ktypes.h"
#include "kernel/tch_kernel.h"
#include "kernel/tch_err.h"
#include "kernel/tch_kobj.h"
#include "kernel/tch_sem.h"
#include "kernel/util/cdsl_dlist.h"


#define TCH_SEMAPHORE_CLASS_KEY                      ((uint16_t) 0x1A0A)


DECLARE_SYSCALL_1(semaphore_create,uint32_t, tch_semId);
DECLARE_SYSCALL_2(semaphore_lock,tch_semId,uint32_t,tchStatus);
DECLARE_SYSCALL_1(semaphore_unlock,tch_semId,tchStatus);
DECLARE_SYSCALL_1(semaphore_destroy,tch_semId,tchStatus);
DECLARE_SYSCALL_2(semaphore_init,tch_semCb*,uint32_t,tchStatus);
DECLARE_SYSCALL_1(semaphore_deinit,tch_semCb*,tchStatus);


static tch_semId tch_semCreate(uint32_t count);
static tchStatus tch_semLock(tch_semId id,uint32_t timeout);
static tchStatus tch_semUnlock(tch_semId id);
static tchStatus tch_semDestroy(tch_semId id);
static tch_semId sem_init(tch_semCb* scb,uint32_t count,BOOL isStatic);
static tchStatus sem_deinit(tch_semCb* scb);



static void tch_semaphoreValidate(tch_semId sid);
static void tch_semaphoreInvalidate(tch_semId sid);
static BOOL tch_semaphoreIsValid(tch_semId sid);





__attribute__((section(".data"))) static tch_semaph_ix Semaphore_StaticInstance = {
		.create = tch_semCreate,
		.lock = tch_semLock,
		.unlock = tch_semUnlock,
		.destroy = tch_semDestroy
};

const tch_semaph_ix* Sem = (const tch_semaph_ix*) &Semaphore_StaticInstance;

DEFINE_SYSCALL_1(semaphore_create,uint32_t,count,tch_semId){
	tch_semCb* semCb = (tch_semCb*) kmalloc(sizeof(tch_semCb));
	tch_semId id = sem_init(semCb,count,FALSE);
	if(!id)
		KERNEL_PANIC("tch_sem.c","can't create semaphore");
	return id;
}


DEFINE_SYSCALL_2(semaphore_lock,tch_semId, semid,uint32_t,timeout,tchStatus){
	if(!semid)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(semid))
		return tchErrorResource;

	tch_semCb* sem = (tch_semCb*) semid;
	tchStatus result = tchOK;
	if(!tch_port_exclusiveCompareDecrement((int*)&sem->count,0)){
		if(!timeout)
			return tchErrorResource;
		tch_schedWait((tch_thread_queue*) &sem->wq,timeout);
	}
	return tchOK;
}


DEFINE_SYSCALL_1(semaphore_unlock,tch_semId,semid,tchStatus){
	if(!semid)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(semid))
		return tchErrorResource;
	tch_semCb* sem = (tch_semCb*) semid;
	sem->count++;
	if(!cdsl_dlistIsEmpty(&sem->wq)){
		tch_schedWake((tch_thread_queue*) &sem->wq, SCHED_THREAD_ALL,tchInterrupted,TRUE);
	}
	return tchOK;
}

DEFINE_SYSCALL_1(semaphore_destroy,tch_semId,semid,tchStatus){
	tchStatus result = sem_deinit(semid);
	if(result != tchOK)
		return result;
	kfree(semid);
	return result;
}

DEFINE_SYSCALL_2(semaphore_init,tch_semCb*,sp,uint32_t,count,tchStatus){
	if(!sp)
		return tchErrorParameter;
	sem_init(sp,count,TRUE);
	return tchOK;
}

DEFINE_SYSCALL_1(semaphore_deinit,tch_semCb*,sp,tchStatus){
	if(!sp || !tch_semaphoreIsValid(sp))
		return tchErrorParameter;
	return sem_deinit(sp);
}


static tch_semId tch_semCreate(uint32_t count){
	if(!count)
		return NULL;		// count value should be larger than zero
	if(tch_port_isISR())
		return NULL;		// semaphore can't be created isr context
	return (tch_semId) __SYSCALL_1(semaphore_create,count);
}

static tchStatus tch_semLock(tch_semId id,uint32_t timeout){
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

static tchStatus tch_semUnlock(tch_semId id){
	if(!tch_semaphoreIsValid(id))
		return tchErrorResource;
	if(tch_port_isISR())
		return __semaphore_unlock(id);
	return __SYSCALL_1(semaphore_unlock,id);
}

static tchStatus tch_semDestroy(tch_semId id){
	if(tch_port_isISR())
		return __semaphore_destroy(id);
	return __SYSCALL_1(semaphore_destroy,id);
}

tch_semId sem_init(tch_semCb* scb,uint32_t count,BOOL isStatic){
	if(!scb || !count)
		return NULL;
	scb->count = count;
	cdsl_dlistInit(&scb->wq);
	tch_registerKobject(&scb->__obj,isStatic? (tch_kobjDestr) sem_deinit : (tch_kobjDestr) tch_semDestroy);
	tch_semaphoreValidate(scb);
	return scb;
}

tchStatus sem_deinit(tch_semCb* scb){
	if(!scb)
		return tchErrorParameter;
	if(!tch_semaphoreIsValid(scb))
		return tchErrorResource;
	scb->state = 0;
	scb->count = 0;
	tch_semaphoreInvalidate(scb);
	if(!cdsl_dlistIsEmpty(&scb->wq)){
		tch_schedWake((tch_thread_queue*) &scb->wq,SCHED_THREAD_ALL,tchErrorResource,FALSE);
	}
	tch_unregisterKobject(&scb->__obj);
	return tchOK;
}

tchStatus tch_semInit(tch_semCb* scb,uint32_t count){
	if(!scb)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __semaphore_init(scb,count);
	return __SYSCALL_2(semaphore_init,scb,count);
}

tchStatus tch_semDeinit(tch_semCb* scb){
	if(!scb)
		return tchErrorParameter;
	if(tch_port_isISR())
		return __semaphore_deinit(scb);
	return __SYSCALL_1(semaphore_deinit,scb);
}


static void tch_semaphoreValidate(tch_semId sid){
	((tch_semCb*) sid)->state |= (((uint32_t) sid & 0xFFFF) ^ TCH_SEMAPHORE_CLASS_KEY);
}

static void tch_semaphoreInvalidate(tch_semId sid){
	((tch_semCb*) sid)->state &= ~(0xFFFF);
}

static BOOL tch_semaphoreIsValid(tch_semId sid){
	return (((tch_semCb*) sid)->state & 0xFFFF) == (((uint32_t) sid & 0xFFFF) ^ TCH_SEMAPHORE_CLASS_KEY);
}
