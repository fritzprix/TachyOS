/*
 * tch_idle.c
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_idle.h"
#include "tch_lwtask.h"



static DECLARE_LWTASK(idleTaskHandler);
static DECLARE_THREADROUTINE(idle);

#define IDLE_CMD_GOSLEEP			((int) 1)

struct idle_parameter {
	int 			cmd;
	void* 			obj;
};

static tch_mtxId	idle_lock;
static uint32_t		busy_cnt;
static tch_threadId idleThread;


void idle_init(){
	tch_threadCfg thcfg;
	Thread->initCfg(&thcfg,idle,Idle,CONFIG_THREAD_MIN_STACK,0,"idle");
	idleThread = Thread->create(&thcfg,NULL);

	busy_cnt = 0;
	idle_lock = tch_rti->Mtx->create();

	tch_rti->Thread->start(idleThread);
}

/**
 *  @brief notify idle thread that there is ongoing hardware task and keep system power up
 */
void idle_set_busy(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt++;
	tch_rti->Mtx->unlock(idle_lock);
}

/**
 * @brief notify ongoing task is finished and can go lower power mode
 */

void idle_clear_busy(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt--;
	tch_rti->Mtx->unlock(idle_lock);
}


static DECLARE_THREADROUTINE(idle){

	int idle_tskid = tch_lwtsk_registerTask(idleTaskHandler,TSK_PRIOR_NORMAL);
	tch_rtcHandle* rtc_handle = tch_rti->Thread->getArg();
	struct idle_parameter parm;

	while(TRUE){
		// some function entering sleep mode
		if((!busy_cnt) && (getThreadKHeader(tch_currentThread)->tslot > 5) && tch_schedIsEmpty()  && tch_systimeIsPendingEmpty()){
			parm.cmd = IDLE_CMD_GOSLEEP;
			parm.obj = rtc_handle;
			tch_lwtsk_request(idle_tskid,&parm,FALSE);
		}
		tch_hal_enterSleepMode();
		// some function waking up from sleep mode
 	}
	return 0;
}


static DECLARE_LWTASK(idleTaskHandler){
	struct idle_parameter* parm;
	switch(parm->cmd){
	case IDLE_CMD_GOSLEEP:
		tch_hal_disableSystick();
		tch_hal_setSleepMode(LP_LEVEL1);
		tch_hal_pauseSysClock();
		tch_hal_enterSleepMode();
		tch_hal_resumeSysClock();
		tch_hal_setSleepMode(LP_LEVEL0);
		tch_hal_enableSystick();
		break;
	}
}
