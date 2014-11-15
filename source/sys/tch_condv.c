/*
 * tch_condv.c
 *
 *  Created on: 2014. 8. 15.
 *      Author: innocentevil
 */

#include "tch.h"
#include "tch_kernel.h"
#include "tch_condv.h"


#define CONDV_VALID        ((uint8_t) 1)
#define CONDV_WAIT         ((uint8_t) 2)

#define TCH_CONDV_CLASS_KEY           ((uint16_t) 0x2D01)

#define tch_condvSetWait(condv)       ((tch_condv*) condv)->state |= CONDV_WAIT
#define tch_condvClrWait(condv)       ((tch_condv*) condv)->state &= ~CONDV_WAIT
#define tch_condvIsWait(condv)        ((tch_condv*) condv)->state & CONDV_WAIT



typedef struct _tch_condv_cb {
	uint32_t          state;
	tch_mtxId         wakeMtx;
	tch_thread_queue  wq;
}tch_condv;

static inline void tch_condvValidate(tch_condvId condv);
static inline void tch_condvInvalidate(tch_condvId condv);
static inline BOOL tch_condvIsValid(tch_condvId condv);

static tch_condvId tch_condv_create();
static tchStatus tch_condv_wait(tch_condvId condv,tch_mtxId mtx,uint32_t timeout);
static tchStatus tch_condv_wake(tch_condvId condv);
static tchStatus tch_condv_wakeAll(tch_condvId condv);
static tchStatus tch_condv_destroy(tch_condvId condv);



__attribute__((section(".data"))) static tch_condv_ix CondVar_StaticInstance = {
		tch_condv_create,
		tch_condv_wait,
		tch_condv_wake,
		tch_condv_wakeAll,
		tch_condv_destroy
};



const tch_condv_ix* Condv = &CondVar_StaticInstance;



static tch_condvId tch_condv_create(){
	tch_condv* condv = (tch_condv*)shMem->alloc(sizeof(tch_condv));
	uStdLib->string->memset(condv,0,sizeof(tch_condv));
	tch_listInit((tch_lnode_t*)&condv->wq);
	condv->wakeMtx = NULL;
	tch_condvValidate(condv);
	return condv;
}

/*! \brief thread wait until given condition is met
 *  \
 *
 */
static tchStatus tch_condv_wait(tch_condvId id,tch_mtxId lock,uint32_t timeout){
	tch_condv* condv = (tch_condv*) id;
	if(!tch_condvIsValid(condv))
		return osErrorResource;
	condv->wakeMtx = lock;
	if(tch_port_isISR()){
		tch_kernel_errorHandler(FALSE,osErrorISR);
		return osErrorISR;
	}
	tch_condvSetWait(condv);
	Mtx->unlock(lock);
	tchStatus result = osOK;
	if((result = tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&condv->wq,timeout)) != osOK){
		if(!tch_condvIsValid(condv))
			return osErrorResource;
		switch(result){
		case osEventTimeout:
			return osErrorTimeoutResource;
		case osErrorResource:
			return osErrorResource;
		}
	}
	if(!tch_condvIsValid(condv))
		return osErrorResource;
	if((result = Mtx->lock(lock,osWaitForever)) != osOK)
		return osErrorResource;
	return osOK;
}





/*!
 * \brief posix condition variable signal
 */
static tchStatus tch_condv_wake(tch_condvId id){
	tch_condv* condv = (tch_condv*) id;
	if(!tch_condvIsValid(condv))
		return osErrorResource;
	if(tch_port_isISR()){                  // if isr mode, no locked mtx is supplied
		if(tch_condvIsWait(condv)){    // check condv is not done
			tch_condvClrWait(condv);   // set condv to done
			tch_port_enterSvFromIsr(SV_THREAD_RESUME,(uint32_t)&condv->wq,osOK);     // wake
			return osOK;
		}else{
			return osErrorParameter;
		}
	}else{
		if(tch_condvIsWait(condv)){
			tch_condvClrWait(condv);
			if(Mtx->unlock(condv->wakeMtx) != osOK)          // if mtx is not locked by this thread
				return osErrorResource;
			tch_port_enterSvFromUsr(SV_THREAD_RESUME,(uint32_t)&condv->wq,osOK);   // wake single thread from wait queue
			return Mtx->lock(condv->wakeMtx,osWaitForever);                        // lock mtx and return
		}
		return osErrorParameter;
	}
}

static tchStatus tch_condv_wakeAll(tch_condvId id){
	tch_condv* condv = (tch_condv*) id;
	if(!tch_condvIsValid(condv))
		return osErrorResource;
	if(tch_port_isISR()){
		if(tch_condvIsWait(condv)){
			tch_condvClrWait(condv);
			tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&condv->wq,osOK); // wake all
			return osOK;
		}else{
			return osErrorParameter;
		}
	}else {
		if(tch_condvIsWait(condv)){
			tch_condvClrWait(condv);
			if(Mtx->unlock(condv->wakeMtx) != osOK)
				return osErrorParameter;
			tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uword_t)&condv->wq,osOK);
			return Mtx->lock(condv->wakeMtx,osWaitForever);
		}
		return osErrorResource;
	}
}

static tchStatus tch_condv_destroy(tch_condvId id){
	tch_condv* condv = (tch_condv*) id;
	tchStatus result = osOK;
	if(!tch_condvIsValid(condv))
		return osErrorResource;
	if(tch_port_isISR()){
		return osErrorISR;
	}else{
		Mtx->lock(condv->wakeMtx,osWaitForever);
		tch_condvInvalidate(condv);
		result = tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uword_t)&condv->wq,osErrorResource);
		Mtx->unlock(condv->wakeMtx);
		shMem->free(condv);
		return result;
	}
}

static inline void tch_condvValidate(tch_condvId condId){
	((tch_condv*) condId)->state |= (((((uint32_t) condId) & 0xFFFF) ^ TCH_CONDV_CLASS_KEY) << 2);
}

static inline void tch_condvInvalidate(tch_condvId condId){
	((tch_condv*) condId)->state &= ~(0xFFFF << 2);
}

static inline BOOL tch_condvIsValid(tch_condvId condv){
	return ((((tch_condv*) condv)->state  >> 2) & 0xFFFF) ==  ((((uint32_t) condv) & 0xFFFF) ^ TCH_CONDV_CLASS_KEY);
}
