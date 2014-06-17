/*
 * tch_thread.c
 *
 *  Created on: 2014. 6. 17.
 *      Author: innocentevil
 */

#include "tch.h"
#include "port/acm4f/tch_port.h"
#include "lib/tch_absdata.h"





static tch_thread_id tch_threadCreate(tch_thread_cfg* cfg,void* arg);
static void tch_threadStart(tch_thread_id thread);
static osStatus tch_threadTerminate(tch_thread_id thread);
static tch_thread_id tch_threadGetCurrent(void);
static osStatus tch_threadSleep(int millisec);
static osStatus tch_threadJoin(tch_thread_id thread);
static void tch_threadSetPriority(tch_thread_prior nprior);
static tch_thread_prior tch_threadGetPriorty();

struct _tch_thread_header_t {
	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_genericList_queue_t*    t_waitQ;
	tch_thread_routine          t_fn;
	const char*                 t_name;
	void*                       t_arg;
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	uint32_t                    t_status;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint32_t                    t_id;
	uint64_t                    t_to;
	tch_thread_context          t_ctx;
};


static tch_thread_ix tch_threadix = {
		tch_threadCreate,
		tch_threadStart,
		tch_threadTerminate,
		tch_threadGetCurrent,
		tch_threadSleep,
		tch_threadJoin,
		tch_threadSetPriority,
		tch_threadGetPriorty
};


const tch_thread_ix* Thread = &tch_threadix;

tch_thread_id tch_threadCreate(tch_thread_cfg* cfg,void* arg){

}

void tch_threadStart(tch_thread_id thread){

}

osStatus tch_threadTerminate(tch_thread_id thread){

}

tch_thread_id tch_threadGetCurrent(void){

}

osStatus tch_threadSleep(int millisec){

}

osStatus tch_threadJoin(tch_thread_id thread){

}

void tch_threadSetPriority(tch_thread_prior nprior){

}

tch_thread_prior tch_threadGetPriorty(){

}
