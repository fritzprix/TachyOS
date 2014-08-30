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
#include "tch_ktypes.h"
#include "tch_bar.h"

#define BAR_INIT_FLAG                ((uint8_t) 0x1)
#define BAR_IS_INIT(bar)             (((tch_barDef*) bar)->status & BAR_INIT_FLAG)
#define BAR_SET_INIT(bar)            ((tch_barDef*) bar)->status |= BAR_INIT_FLAG
#define BAR_CLR_INIT(bar)            ((tch_barDef*) bar)->status &= ~BAR_INIT_FLAG

static tch_barId tch_bar_create(tch_barDef* bar);
static tchStatus tch_bar_wait(tch_barDef* bar,uint32_t timeout);
static tchStatus tch_bar_signal(tch_barDef* bar,tchStatus result);
static tchStatus tch_bar_destroy(tch_barDef* bar);



__attribute__((section(".data"))) static tch_bar_ix Barrier_StaticInstance = {
		tch_bar_create,
		tch_bar_wait,
		tch_bar_signal,
		tch_bar_destroy
};

const tch_bar_ix* Barrier = &Barrier_StaticInstance;


static tch_barId tch_bar_create(tch_barDef* bar){
	if(!bar)
		return osErrorParameter;
	if(BAR_IS_INIT(bar)){
		return ((tch_barId)0);
	}
	BAR_SET_INIT(bar);
	tch_listInit(&bar->wq);
	return ((tch_barId)bar);
}

static tchStatus tch_bar_wait(tch_barDef* bar,uint32_t timeout){
	if(!bar)
		return osErrorParameter;
	if(!BAR_IS_INIT(bar))
		return osErrorResource;
	if(tch_port_isISR())
		return osErrorISR;
	return tch_port_enterSvFromUsr(SV_THREAD_SUSPEND,(uint32_t)&bar->wq,timeout);
}

static tchStatus tch_bar_signal(tch_barDef* bar,tchStatus result){
	if(!bar)
		return osErrorParameter;
	if(!BAR_IS_INIT(bar))
		return osErrorResource;
	if(tch_listIsEmpty(&bar->wq))
		return osOK;
	if(tch_port_isISR()){
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osOK);
		return osOK;
	}
	return tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osOK);
}

static tchStatus tch_bar_destroy(tch_barDef* bar){
	if((!bar) || (!BAR_IS_INIT(bar)))
		return osErrorParameter;
	if(tch_listIsEmpty(&bar->wq))
		return osOK;
	BAR_CLR_INIT(bar);
	if(tch_port_isISR())
		tch_port_enterSvFromIsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osErrorResource);
	else
		tch_port_enterSvFromUsr(SV_THREAD_RESUMEALL,(uint32_t)&bar->wq,osErrorResource);
	return osOK;
}
