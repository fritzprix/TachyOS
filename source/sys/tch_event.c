/*
 * tch_event.c
 *
 *  Created on: 2014. 11. 8.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_event.h"
#include "tch_ltree.h"


typedef struct tch_event_node_t {
	tch_ltree_node          tn;
	const tch*              env;
	int                     id;
	tch_eventCallback       cb;
	void*                   en_arg;
	const char*             en_name;
}tch_eventNode;



static tch_eventNode tch_eventCreate(const tch* env,const char* name);
static tchStatus tch_eventListen(tch_eventNode evnode,int id, tch_eventCallback cb,uint32_t timeout);
static tchStatus tch_eventNotify(tch_eventNode evnode,int id, void* ev_arg);
static tchStatus tch_eventDestroy(tch_eventNode evnode);

__attribute__((section(".data")))static tch_event_ix Event_StaticInstance = {
		tch_eventCreate,
		tch_eventListen,
		tch_eventNotify,
		tch_eventDestroy
};

const tch_event_ix* Event = (const tch_event_ix*) &Event_StaticInstance;




static tch_eventNode tch_eventCreate(const tch* env,const char* name){

}

static tchStatus tch_eventListen(tch_eventNode evnode,int id, tch_eventCallback cb,uint32_t timeout){

}

static tchStatus tch_eventNotify(tch_eventNode evnode,int id, void* ev_arg){

}

static tchStatus tch_eventDestroy(tch_eventNode evnode){

}
