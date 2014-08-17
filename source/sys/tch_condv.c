/*
 * tch_condv.c
 *
 *  Created on: 2014. 8. 15.
 *      Author: innocentevil
 */

#include "tch.h"
#include "tch_ktypes.h"
#include "tch_condv.h"


#define CONDV_VALID        ((uint8_t) 1)
#define CONDV_WAIT         ((uint8_t) 2)

#define VALIDATE(condv)   {((tch_condvType*) condv)->state |= CONDV_VALID;}
#define INVALIDATE(condv) {((tch_condvType*) condv)->state &= ~CONDV_VALID;}
#define IS_VALID(condv)   (((tch_condvType*) condv)->state & CONDV_VALID)

#define SETWAIT(condv)    {((tch_condvType*) condv)->state |= CONDV_WAIT;}
#define CLRWAIT(condv)    {((tch_condvType*) condv)->state &= ~CONDV_WAIT;}
#define IS_WAITTING(condv) (((tch_condvType*) condv)->state & CONDV_WAIT)


typedef struct _tch_condv_type {
	uint8_t          state;
	tch_mtx*         wakeMtx;
	tch_thread_queue wq;
}tch_condvType;


static tch_condv_id tch_condv_create();
static BOOL tch_condv_wait(tch_condv_id condv,uint32_t timeout);
static tchStatus tch_condv_wake(tch_condv_id condv);
static tchStatus tch_condv_wakeAll(tch_condv_id condv);
static tchStatus tch_condv_destroy(tch_condv_id condv);



__attribute__((section(".data"))) static tch_condv_ix CondVar_StaticInstance = {
		tch_condv_create,
		tch_condv_wait,
		tch_condv_wake,
		tch_condv_wakeAll,
		tch_condv_destroy
};



const tch_condv_ix* pcondvar = &CondVar_StaticInstance;



static tch_condv_id tch_condv_create(){
	tch_condvType* condv = (tch_condvType*)Mem->alloc(sizeof(tch_condvType));
	tch_listInit(&condv->wq);
	condv->wakeMtx = NULL;
	VALIDATE(condv);
	return condv;
}

/**
 *  thread wait until given condition is met
 *
 */
static BOOL tch_condv_wait(tch_condv_id id,tch_mtx* lock,uint32_t timeout){
	if(!IS_VALID(id))
		return osErrorResource;
	tch_condvType* condv = (tch_condvType*) id;
	condv->wakeMtx = lock;
	if(__get_IPSR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return FALSE;
	}
	SETWAIT(condv);
	Mtx->unlock(lock);
	if(tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,&condv->wq,timeout) != osOK){
		return FALSE;
	}
	if(Mtx->lock(lock,osWaitForever) != osOK)
		return FALSE;
	return TRUE;
}


/*!
 * \brief posix condition variable signal
 */
static tchStatus tch_condv_wake(tch_condv_id id){
	if(!IS_VALID(id))
		return osErrorResource;
	tch_condvType* condv = (tch_condvType*) id;
	if(__get_IPSR()){               // if isr mode, no locked mtx is supplied
		if(IS_WAITTING(condv)){           // check condv is not done
			CLRWAIT(condv);     // set condv to done
			tch_port_enterSvFromIsr(SV_THREAD_RESUME,&condv->wq,osOK);     // wake
			return osOK;
		}else{
			return osErrorParameter;
		}
	}else{
		if(IS_WAITTING(condv)){
			CLRWAIT(condv);
			if(Mtx->unlock(condv->wakeMtx) != osOK)          // if mtx is not locked by this thread
				return osErrorResource;
			tch_port_enterSvFromUsr(SV_THREAD_RESUME,&condv->wq,osOK);         // wake single thread from wait queue
			return Mtx->lock(condv->wakeMtx,osWaitForever);                 // lock mtx and return
		}
		return osErrorParameter;                                       //
	}
}

static tchStatus tch_condv_wakeAll(tch_condv_id id){
	if(!IS_VALID(id))
		return osErrorResource;
	tch_condvType* condv = (tch_condvType*) id;
	if(__get_IPSR()){
		if(IS_WAITTING(condv)){
			CLRWAIT(condv);
			tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,&condv->wq,osOK); // wake all
			return osOK;
		}else{
			return osErrorParameter;
		}
	}else {
		if(IS_WAITTING(condv)){
			CLRWAIT(condv);
			if(Mtx->unlock(condv->wakeMtx) != osOK)
				return osErrorParameter;
			tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,&condv->wq,osOK);
			return Mtx->lock(condv->wakeMtx,osWaitForever);
		}
		return osErrorResource;
	}
}

static tchStatus tch_condv_destroy(tch_condv_id id){
	if(!IS_VALID(id))
		return osErrorResource;
	tch_condvType* condv = (tch_condvType*) id;
	if(__get_IPSR()){
		INVALIDATE(condv);
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,&condv->wq,osErrorResource);
		return osOK;
	}else{
		Mtx->lock(condv->wakeMtx,osWaitForever);
		INVALIDATE(condv);
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,&condv->wq,osErrorResource);
		return osOK;
	}
}
