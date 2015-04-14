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


static BOOL consume(const tch* api,struct VBuf* vb,uint32_t timeout);
static BOOL produce(const tch* api,struct VBuf* vb,uint32_t timeout);

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


	tch_assert(api,cons1stk && cons2stk && prod1stk && prod2stk,tchErrorOS);

	tch_threadCfg thcfg;
	thcfg.t_name = "consumer1";
	thcfg.t_routine = consumerRoutine;
	thcfg.t_priority = Normal;
	thcfg.t_memDef.stk_sz = 512;

	consumer1Thread = api->Thread->create(&thcfg,api);

	thcfg.t_name = "consumer2";
	consumer2Thread = api->Thread->create(&thcfg,api);



	thcfg.t_name = "producer1";
	thcfg.t_routine = producerRoutine;
	thcfg.t_priority = Normal;
	thcfg.t_memDef.stk_sz = 512;

	producer1Thread = api->Thread->create(&thcfg,api);

	thcfg.t_name = "producer2";
	producer2Thread = api->Thread->create(&thcfg,api);


	api->Thread->start(producer1Thread);
	api->Thread->start(producer2Thread);

	api->Thread->start(consumer1Thread);
	api->Thread->start(consumer2Thread);




	if(api->Thread->join(producer1Thread,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(api->Thread->join(producer2Thread,tchWaitForever) != tchOK)
		return tchErrorOS;


	api->Mtx->destroy(mtid);
	api->Condv->destroy(condP);
	api->Condv->destroy(condC);


	if(api->Thread->join(consumer1Thread,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(api->Thread->join(consumer2Thread,tchWaitForever) != tchOK)
		return tchErrorOS;


	api->Condv->destroy(condP);
	api->Condv->destroy(condC);

	if(tstBuf.updated == 0)
		return tchOK;
	return tchErrorOS;

}


static BOOL consume(const tch* api,struct VBuf* vb,uint32_t timeout){
	if(api->Mtx->lock(mtid,tchWaitForever) != tchOK)
		return FALSE;
	while(vb->updated == 0){
		if(api->Condv->wait(condC,mtid,timeout) != tchOK)
			return FALSE;
	}
	vb->updated--;
	api->Condv->wakeAll(condP);
	return api->Mtx->unlock(mtid) == tchOK;
}

static BOOL produce(const tch* api,struct VBuf* vb,uint32_t timeout){
	 if(api->Mtx->lock(mtid,tchWaitForever) != tchOK)
		 return FALSE;
	 while(vb->updated == vb->size){
		 if(api->Condv->wait(condP,mtid,timeout) != tchOK)
			 return FALSE;
	 }
	 vb->updated++;
	 api->Condv->wakeAll(condC);
	 return api->Mtx->unlock(mtid) == tchOK;
}



static DECLARE_THREADROUTINE(producerRoutine){
	uint8_t cnt = 0;
	for(cnt = 0; cnt < 200; cnt++){
		produce(env,&tstBuf,tchWaitForever);
		env->Thread->yield(0);
	}
	return tchOK;
}

static DECLARE_THREADROUTINE(consumerRoutine){
	uint8_t cnt = 0;
	for(cnt = 0; cnt < 200; cnt++){
		consume(env,&tstBuf,tchWaitForever);
		env->Thread->yield(0);
	}
	if(!consume(env,&tstBuf,tchWaitForever))
		return tchOK;
	return tchErrorOS;
}


