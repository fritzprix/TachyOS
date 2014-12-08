

#include "tch_event.h"
#include "tch_ltree.h"


typedef struct tch_event_node_t {
	tch_uobjDestr           Destr;
	tch_ltree_node          ev_ltn;
	tch_eventHandler        ev_hdlr;
	tch_thread_queue        ev_wq;
}tch_eventNodePrototype;


typedef struct tch_event_root_t {
	tch_uobjDestr           Destr;
	tch_ltree_node          ev_ltn;
	uint32_t                ev_status;
}tch_eventNodeRoot;



static tch_eventTree* tch_evTreeCreate();
static void tch_evTreeDestroy(tch_eventTree* self);
static tchStatus tch_evTreeAddEventNode(tch_eventTree* self,int ev_id,tch_eventHandler ev_handler);
static tchStatus tch_evTreeRemoveEventNode(tch_eventTree* self,int ev_id);
static tchStatus tch_evTreeWaitEvent(tch_eventTree* self,int ev_id,uint32_t timeout);
static tchStatus tch_evTreeRaiseEvent(tch_eventTree* self,int ev_id,int ev_msg);
static tchStatus tch_evTreeRaiseAllEvents(tch_eventTree* self,int ev_msg);

__attribute__((section(".data"))) static tch_event_ix Event_StaticInstance = {
		tch_evTreeCreate
};

const tch_event_ix* Event = &Event_StaticInstance;



static tch_eventTree* tch_evTreeCreate(){

}

static void tch_evTreeDestroy(tch_eventTree* self){

}

static tchStatus tch_evTreeAddEventNode(tch_eventTree* self,int ev_id,tch_eventHandler ev_handler){

}

static tchStatus tch_evTreeRemoveEventNode(tch_eventTree* self,int ev_id){

}

static tchStatus tch_evTreeWaitEvent(tch_eventTree* self,int ev_id,uint32_t timeout){

}

static tchStatus tch_evTreeRaiseEvent(tch_eventTree* self,int ev_id,int ev_msg){

}

static tchStatus tch_evTreeRaiseAllEvents(tch_eventTree* self,int ev_msg){

}
