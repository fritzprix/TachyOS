/*
 * tch_event.c
 *
 *  Created on: 2014. 11. 8.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_event.h"
#include "tch_ltree.h"



struct tch_event_prototype_t {
	tch_event_ix            ev_pix;
	tch_ltree_node          ev_tree;
};

typedef struct tch_event_node_t {
	tch_ltree_node          tn;
	tch_sysTask             cb_task;
}tch_eventNode;


static tchStatus tch_eventListen(const tch* env,int id, tch_eventCallback cb,uint32_t timeout);
static tchStatus tch_eventNotify(const tch* env,int id, void* ev_arg);

__attribute__((section(".data")))static struct tch_event_prototype_t Event_StaticInstance = {
		{tch_eventListen,tch_eventNotify},
		INIT_LTREE
};

const tch_event_ix* Event = (const tch_event_ix*) &Event_StaticInstance;

static tchStatus tch_eventListen(const tch* env,int id, tch_eventCallback cb,uint32_t timeout){
	tch_eventNode* evNode = (tch_eventNode*) env->Mem->alloc(sizeof(tch_eventNode));
	evNode->cb_task.id = id;
	evNode->cb_task.fn = cb;
	evNode->cb_task.prior = tch_currentThread->t_prior;
	evNode->cb_task.status = osOK;
	evNode->cb_task.arg = NULL;

	// critical section
	tch_ltreeInit(&evNode->tn,id);

}

static tchStatus tch_eventNotify(const tch* env,int id, void* ev_arg){

}
