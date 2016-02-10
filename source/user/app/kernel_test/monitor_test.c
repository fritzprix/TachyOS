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


static BOOL consume(const tch_core_api_t* api,struct VBuf* vb,uint32_t timeout);
static BOOL produce(const tch_core_api_t* api,struct VBuf* vb,uint32_t timeout);

static tch_mtxId mtid;

static tch_condvId condP;
static tch_condvId condC;

static tch_threadId consumer1Thread;
static tch_threadId consumer2Thread;

static tch_threadId producer1Thread;
static tch_threadId producer2Thread;


tchStatus monitor_performTest(tch_core_api_t* ctx){
	mstat init_mstat,fin_mstat;

	kmstat(&init_mstat);
	tstBuf.size = 256;
	tstBuf.updated = 0;

	mtid = ctx->Mtx->create();
	condP = ctx->Condv->create();
	condC = ctx->Condv->create();


	thread_config_t thcfg;
	ctx->Thread->initConfig(&thcfg,consumerRoutine,Normal,512,0,"consumer1");
	consumer1Thread = ctx->Thread->create(&thcfg,ctx);

	ctx->Thread->initConfig(&thcfg,consumerRoutine,Normal,512,0,"consumer2");
	consumer2Thread = ctx->Thread->create(&thcfg,ctx);


	ctx->Thread->initConfig(&thcfg,producerRoutine,Normal,512,0,"producer1");
	producer1Thread = ctx->Thread->create(&thcfg,ctx);

	ctx->Thread->initConfig(&thcfg,producerRoutine,Normal,512,0,"producer2");
	producer2Thread = ctx->Thread->create(&thcfg,ctx);


	ctx->Thread->start(producer1Thread);
	ctx->Thread->start(producer2Thread);

	ctx->Thread->start(consumer1Thread);
	ctx->Thread->start(consumer2Thread);




	if(ctx->Thread->join(producer1Thread,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Thread->join(producer2Thread,tchWaitForever) != tchOK)
		return tchErrorOS;


	ctx->Mtx->destroy(mtid);
	ctx->Condv->destroy(condP);
	ctx->Condv->destroy(condC);


	if(ctx->Thread->join(consumer1Thread,tchWaitForever) != tchOK)
		return tchErrorOS;
	if(ctx->Thread->join(consumer2Thread,tchWaitForever) != tchOK)
		return tchErrorOS;


	ctx->Condv->destroy(condP);
	ctx->Condv->destroy(condC);

	kmstat(&fin_mstat);

	if(init_mstat.used != fin_mstat.used)
		return tchErrorMemoryLeaked;
	if(tstBuf.updated == 0)
		return tchOK;

	return tchErrorOS;

}


static BOOL consume(const tch_core_api_t* api,struct VBuf* vb,uint32_t timeout){
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

static BOOL produce(const tch_core_api_t* api,struct VBuf* vb,uint32_t timeout){
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
		produce(ctx,&tstBuf,tchWaitForever);
		ctx->Thread->yield(0);
	}
	return tchOK;
}

static DECLARE_THREADROUTINE(consumerRoutine){
	uint8_t cnt = 0;
	for(cnt = 0; cnt < 200; cnt++){
		consume(ctx,&tstBuf,tchWaitForever);
		ctx->Thread->yield(0);
	}
	if(!consume(ctx,&tstBuf,tchWaitForever))
		return tchOK;
	return tchErrorOS;
}


