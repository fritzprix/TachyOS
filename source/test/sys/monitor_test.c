/*
 * monitor_test.c
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */


/*!brief Unit test for monitor(Mutex & Condv)
 *
 */

#include "tch.h"
#include "monitor_test.h"


#define MAX_SZ    ((uint32_t) -1)
#define MIN_SZ    ((uint32_t) 0)

struct VBuf {
	uint32_t size;
	uint32_t updated;
};

static struct VBuf tstBuf;

static DECLARE_THREADROUTINE(producerRoutine);
static DECLARE_THREADROUTINE(consumerRoutine);


static BOOL consume(tch* api,struct VBuf* vb,uint32_t timeout);
static BOOL produce(tch* api,struct VBuf* vb,uint32_t timeout);

static tch_mtxId mtid;

static tch_condvId condP;
static tch_condvId condC;

static tch_threadId consumer1Thread;
static tch_threadId consumer2Thread;

static tch_threadId producer1Thread;
static tch_threadId producer2Thread;


tchStatus monitor_performTest(tch* api){
	tstBuf.size = 256;
	tstBuf.updated = 0;

	mtid = api->Mtx->create();
	condP = api->Condv->create();
	condC = api->Condv->create();

	uint8_t* cons1stk = NULL;
	uint8_t* cons2stk = NULL;

	uint8_t* prod1stk = NULL;
	uint8_t* prod2stk = NULL;

	cons1stk = api->Mem->alloc(512);
	cons2stk = api->Mem->alloc(512);

	prod1stk = api->Mem->alloc(512);
	prod2stk = api->Mem->alloc(512);

	tch_assert(api,cons1stk && cons2stk && prod1stk && prod2stk,osErrorOS);

	tch_threadCfg thcfg;
	thcfg._t_name = "consumer1";
	thcfg._t_routine = consumerRoutine;
	thcfg._t_stack = cons1stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 512;

	consumer1Thread = api->Thread->create(&thcfg,api);

	thcfg._t_name = "consumer2";
	thcfg._t_stack = cons2stk;
	consumer2Thread = api->Thread->create(&thcfg,api);



	thcfg._t_name = "producer1";
	thcfg._t_routine = producerRoutine;
	thcfg._t_stack = prod1stk;
	thcfg.t_proior = Normal;
	thcfg.t_stackSize = 512;

	producer1Thread = api->Thread->create(&thcfg,api);

	thcfg._t_name = "producer2";
	thcfg._t_stack = prod2stk;
	producer2Thread = api->Thread->create(&thcfg,api);


	api->Thread->start(producer1Thread);
	api->Thread->start(producer2Thread);

	api->Thread->start(consumer1Thread);
	api->Thread->start(consumer2Thread);




	if(api->Thread->join(producer1Thread,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(producer2Thread,osWaitForever) != osOK)
		return osErrorOS;


	api->Mtx->destroy(mtid);
	api->Condv->destroy(condP);
	api->Condv->destroy(condC);


	if(api->Thread->join(consumer1Thread,osWaitForever) != osOK)
		return osErrorOS;
	if(api->Thread->join(consumer2Thread,osWaitForever) != osOK)
		return osErrorOS;

	api->Mem->free(cons1stk);
	api->Mem->free(cons2stk);
	api->Mem->free(prod1stk);
	api->Mem->free(prod2stk);


	api->Condv->destroy(condP);
	api->Condv->destroy(condC);

	if(tstBuf.updated == 0)
		return osOK;
	return osErrorOS;

}


static BOOL consume(tch* api,struct VBuf* vb,uint32_t timeout){
	if(api->Mtx->lock(mtid,osWaitForever) != osOK)
		return FALSE;
	while(vb->updated == 0){
		if(api->Condv->wait(condC,mtid,timeout) != osOK)
			return FALSE;
	}
	vb->updated--;
	api->Condv->wakeAll(condP);
	return api->Mtx->unlock(mtid) == osOK;
}

static BOOL produce(tch* api,struct VBuf* vb,uint32_t timeout){
	 if(api->Mtx->lock(mtid,osWaitForever) != osOK)
		 return FALSE;
	 while(vb->updated == vb->size){
		 if(api->Condv->wait(condP,mtid,timeout) != osOK)
			 return FALSE;
	 }
	 vb->updated++;
	 api->Condv->wakeAll(condC);
	 return api->Mtx->unlock(mtid) == osOK;
}



static DECLARE_THREADROUTINE(producerRoutine){
	uint8_t cnt = 0;
	for(cnt = 0; cnt < 200; cnt++){
		produce(env,&tstBuf,osWaitForever);
		env->Thread->sleep(0);
	}
	return osOK;
}

static DECLARE_THREADROUTINE(consumerRoutine){
	uint8_t cnt = 0;
	for(cnt = 0; cnt < 200; cnt++){
		consume(env,&tstBuf,osWaitForever);
		env->Thread->sleep(0);
	}
	if(!consume(env,&tstBuf,osWaitForever))
		return osOK;
	return osErrorOS;
}


