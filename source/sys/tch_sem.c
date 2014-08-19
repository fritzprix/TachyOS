/*
 * tch_sem.c
 *
 *  Created on: 2014. 8. 19.
 *      Author: innocentevil
 */


#include "tch_ktypes.h"
#include "tch_kernel.h"
#include "tch_sem.h"
#include "tch_list.h"


#define SET_INIT(sem_p)          ((tch_semDef*)sem_p)->key = tch_semaphore_magicNumber
#define IS_INIT(sem_p)           ((((tch_semDef*)sem_p)->key) == tch_semaphore_magicNumber)

static tch_sem_id tch_semaphore_create(tch_semDef* sem,uint32_t count);
static tchStatus tch_semaphore_lock(tch_sem_id sid,uint32_t timeout);
static tchStatus tch_semaphore_unlock(tch_sem_id sid);
static tchStatus tch_semaphore_destroy(tch_sem_id sid);



static const uint32_t tch_semaphore_magicNumber = 0x1A;


__attribute__((section(".data"))) static tch_semaph_ix Semaphore_StaticInstance = {
		tch_semaphore_create,
		tch_semaphore_lock,
		tch_semaphore_unlock,
		tch_semaphore_destroy
};

const tch_semaph_ix* Sem = (const tch_semaph_ix*) &Semaphore_StaticInstance;



static tch_sem_id tch_semaphore_create(tch_semDef* sem,uint32_t count){
	sem->count = count;
	SET_INIT(sem);
	tch_listInit(&sem->wq);
	return (tch_sem_id) sem;
}

static tchStatus tch_semaphore_lock(tch_sem_id id,uint32_t timeout){
	tch_kAssert(IS_INIT(id),osErrorParameter);
	tch_kAssert(!tch_port_isISR(),osErrorISR);

	tch_semDef* sem = (tch_semDef*) id;
	tchStatus result = osOK;
	while(tch_port_exclusiveCompareDecrement((int*)&sem->count,0)){                // try to exclusively decrement count value
		result = tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&sem->wq,timeout);
		switch(result){
		case osEventTimeout:                                   // if timeout expires, sv call will return osEventTimeout
			return osErrorTimeoutResource;
		case osErrorResource:                                  // if semaphore destroyed sv call will return osErrorResource
			return osErrorResource;
		}
	}
	if(result == osOK)
		return osOK;
	tch_kAssert(TRUE,osErrorOS);
	return osErrorOS;                      // unreachable code
}

static tchStatus tch_semaphore_unlock(tch_sem_id id){
	tch_kAssert(IS_INIT(id),osErrorParameter);
	tch_semDef* sem = (tch_semDef*) id;
	sem->count++;
	if(!tch_listIsEmpty(&sem->wq)){
		if(!tch_port_isISR())
			tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&sem->wq,osOK);
		else
			tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&sem->wq,osOK);
	}
	return osOK;
}

static tchStatus tch_semaphore_destroy(tch_sem_id id){
	tch_kAssert(IS_INIT(id),osErrorParameter);

	tch_semDef* sem = (tch_semDef*) id;
	sem->key = 0;
	sem->count = 0;
	if(!tch_listIsEmpty(&sem->wq)){
		if(tch_port_isISR())
			tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&sem->wq,osErrorResource);
		else
			tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&sem->wq,osErrorResource);
	}
	return  osOK;
}
