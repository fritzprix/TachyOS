/*
 * tch_bar.c
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */




/*!\brief Barrier Implementation
 *
 */
#include "tch.h"
#include "tch_kernel.h"
#include "tch_bar.h"


#define TCH_BARRIER_CLASS_KEY        ((uhword_t) 0x2D03)
#define tch_barValidate(bar)         ((tch_barDef*) bar)->state = (((uint32_t) bar & 0xFFFF) ^ TCH_BARRIER_CLASS_KEY)
#define tch_barInvalidate(bar)       ((tch_barDef*) bar)->state &= ~(0xFFFF)
#define tch_barIsValid(bar)          ((tch_barDef*) bar)->state & 0xFFFF == (((uint32_t) bar & 0xFFFF) ^ TCH_BARRIER_CLASS_KEY)
/*
#define BAR_INIT_FLAG                ((uint8_t) 0x1)
#define BAR_IS_INIT(bar)             (((tch_barDef*) bar)->state & BAR_INIT_FLAG)
#define BAR_SET_INIT(bar)            ((tch_barDef*) bar)->state |= BAR_INIT_FLAG
#define BAR_CLR_INIT(bar)            ((tch_barDef*) bar)->state &= ~BAR_INIT_FLAG*/

typedef struct tch_bar_def_t{
	uint32_t                 state;
	tch_lnode_t             wq;
} tch_barDef;

static tch_barId tch_bar_create();
static tchStatus tch_bar_wait(tch_barId bar,uint32_t timeout);
static tchStatus tch_bar_signal(tch_barId bar,tchStatus result);
static tchStatus tch_bar_destroy(tch_barId bar);



__attribute__((section(".data"))) static tch_bar_ix Barrier_StaticInstance = {
		tch_bar_create,
		tch_bar_wait,
		tch_bar_signal,
		tch_bar_destroy
};

const tch_bar_ix* Barrier = &Barrier_StaticInstance;


static tch_barId tch_bar_create(){
	tch_barDef* bar = Mem->alloc(sizeof(tch_barDef));
	if(!bar)
		return NULL;
	if(tch_barIsValid(bar)){
		Mem->free(bar);
		return ((tch_barId)0);
	}
	tch_barValidate(bar);
	tch_listInit(&bar->wq);
	return ((tch_barId)bar);
}

static tchStatus tch_bar_wait(tch_barId bar,uint32_t timeout){
	if(!bar)
		return osErrorParameter;
	if(!tch_barIsValid(bar))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	return tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&((tch_barDef*)bar)->wq,timeout);
}

static tchStatus tch_bar_signal(tch_barId barId,tchStatus result){
	tch_barDef* bar = (tch_barDef*) barId;
	if(!bar)
		return osErrorParameter;
	if(!tch_barIsValid(bar))
		return osErrorResource;
	if(tch_listIsEmpty(&bar->wq))
		return osOK;
	if(tch_port_isISR()){
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osOK);
		return osOK;
	}
	return tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osOK);
}

static tchStatus tch_bar_destroy(tch_barId barId){
	tch_barDef* bar = (tch_barDef*) barId;
	if((!bar) || (!tch_barIsValid(bar)))
		return osErrorParameter;
	if(tch_listIsEmpty(&bar->wq))
		return osOK;
	tch_barInvalidate(bar);
	if(tch_port_isISR())
		return osErrorISR;
	else
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osErrorResource);
	Mem->free(bar);
	return osOK;
}
