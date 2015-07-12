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


void tch_idleInit(){
	tch_threadCfg thcfg;
	Thread->initCfg(&thcfg);
	thcfg.t_routine = (tch_thread_routine) idle;
	thcfg.t_priority = Idle;
	thcfg.t_name = "idle";
	thcfg.t_memDef.heap_sz = 0x200;
	thcfg.t_memDef.stk_sz = TCH_CFG_THREAD_STACK_MIN_SIZE;
	idleThread = Thread->create(&thcfg,NULL);

	busy_cnt = 0;
	idle_lock = tch_rti->Mtx->create();

	tch_rti->Thread->start(idleThread);
}

void tch_kernelSetBusyMark(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt++;
	tch_rti->Mtx->unlock(idle_lock);
}

void tch_kernelClrBusyMark(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt--;
	tch_rti->Mtx->unlock(idle_lock);
}

BOOL tch_kernelIsBusy(){
	return !busy_cnt;
}


static DECLARE_THREADROUTINE(idle){

	int idle_tskid = tch_lwtsk_registerTask(idleTaskHandler,TSK_PRIOR_NORMAL);
	tch_rtcHandle* rtc_handle = tch_rti->Thread->getArg();
	struct idle_parameter parm;

	while(TRUE){
		// some function entering sleep mode
		if((!busy_cnt) && (getThreadKHeader(tch_currentThread)->t_tslot > 5) && tch_schedIsEmpty()  && tch_systimeIsPendingEmpty()){
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
		tch_hal_suspendSysClock();
		tch_hal_enterSleepMode();
		tch_hal_resumeSysClock();
		tch_hal_setSleepMode(LP_LEVEL0);
		tch_hal_enableSystick();
		break;
	}
}
