/*
 * fmo_time.c
 *
 *  Created on: 2014. 4. 6.
 *      Author: innocentevil
 */



#include "fmo_time.h"
#include "../lld/fmo_timer.h"
#include "port/cortex_v7m_port.h"


typedef struct _sys_time_prototype vt_prototype;

#define TIMER_COUNT_MASK                (uint32_t) (0xFF << 0)
#define SET_TIMER_COUNT(stp,cnt) (stp->p_flag |= (TIMER_COUNT_MASK & cnt))
#define GET_TIMER_COUNT(stp)     (stp->p_flag & TIMER_COUNT_MASK)
struct _sys_time_prototype{
	struct _system_time_t               p_ins;
	mtx_lock                            p_lock;
	uint32_t                            p_flag;
	tch_gptimer_instance*                     p_timers[4];
	timeEventListener                   p_listener;
};



#define VT_INTERFACE {tch_vtstart,tch_vtsetTimeout,tch_vtstop}

static int tch_vtstart(timeEventListener listener);
static int tch_vtsetTimeout(uint32_t r_id,uint16_t timeIn_ms);
static int tch_vtstop(void);


static vt_prototype VT_Ins = {
		VT_INTERFACE,
		MTX_INIT,
		0,
		{NULL,NULL,NULL,NULL}
};

static tch_timer VT_Timers[] = {
		0,
		1,
		3,
		4
};

static void gpt_listener(tch_gptimer_instance* dd,uint32_t id);

const vtimer_t* vtimer = (vtimer_t*) &VT_Ins;


int tch_vtstart(timeEventListener listener){

	//timer initialize
	uint8_t t_cnt = sizeof(VT_Timers) / sizeof(tch_timer);
	SET_TIMER_COUNT((&VT_Ins),t_cnt);
	while(t_cnt--){
		VT_Ins.p_timers[t_cnt] = lld_timer_openGPTimer(VT_Timers[t_cnt],500,gpt_listener,ActOnSleep);
	}

	VT_Ins.p_listener = listener;

	return 0;
}

int tch_vtsetTimeout(uint32_t r_id,uint16_t timeIn_ms){
	uint8_t idx = 0;
	while(idx < GET_TIMER_COUNT((&VT_Ins))){
		tch_gptimer_instance* timer = VT_Ins.p_timers[idx++];
		if(timer->setTimeout(timer,r_id,timeIn_ms) == SUCCESS){
			return SUCCESS;
		}
	}
	return ERROR;
}

int tch_vtstop(void){
	uint8_t idx = 0;
	while(idx < GET_TIMER_COUNT((&VT_Ins))){
		tch_gptimer_instance* timer = VT_Ins.p_timers[idx++];
		timer->close(timer);
	}
	return SUCCESS;
}

void gpt_listener(tch_gptimer_instance* dd,uint32_t id){
	if(VT_Ins.p_listener != NULL){
		VT_Ins.p_listener(id);
	}
}
