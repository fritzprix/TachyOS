/*
 * tch_idle.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */


#include "kernel/tch_kernel.h"
#include "kernel/tch_idle.h"
#include "kernel/tch_lwtask.h"
#include "kernel/tch_err.h"

extern DECLARE_THREADROUTINE(main)__attribute__((weak,alias("__main")));

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
static tch_threadId mainThread;



void idle_init(){
	thread_config_t thcfg;
	Thread->initConfig(&thcfg,idle,Idle,USER_MIN_STACK,0,"idle");
	idleThread = (tch_threadId) tch_thread_createThread(&thcfg,NULL,FALSE,TRUE,NULL);

	busy_cnt = 0;
	idle_lock = tch_rti->Mtx->create();
	tch_rti->Thread->start(idleThread);
}

/**
 *  @brief notify idle thread that there is ongoing hardware task and keep system power up
 */
void set_system_busy(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt++;
	tch_rti->Mtx->unlock(idle_lock);
}

/**
 * @brief notify ongoing task is finished and can go lower power mode
 */

void clear_system_busy(){
	if(tch_rti->Mtx->lock(idle_lock,tchWaitForever) != tchOK)
		return;
	busy_cnt--;
	tch_rti->Mtx->unlock(idle_lock);
}


static DECLARE_THREADROUTINE(idle){

	int idle_tskid = tch_lwtsk_registerTask(idleTaskHandler,TSK_PRIOR_NORMAL);
	tch_rtcHandle* rtc_handle = tch_rti->Thread->getArg();
	struct idle_parameter parm;


	thread_config_t threadcfg;
	Thread->initConfig(&threadcfg,main,Normal,0x800,0x800,"main");
	mainThread = (tch_threadId) tch_thread_createThread(&threadcfg,ctx,TRUE,TRUE,NULL);


	if((!mainThread))
		KERNEL_PANIC("Can't create init thread");
	Thread->start(mainThread);
	print_dbg("- Idle loop start\n\r");

	while(TRUE)
	{
		tch_dbg_flush();
		// some function entering sleep mode
		if((!busy_cnt) && (get_thread_kheader(current)->tslot > 5) && tch_schedIsEmpty()  && tch_systimeIsPendingEmpty()){
			parm.cmd = IDLE_CMD_GOSLEEP;
			parm.obj = rtc_handle;
			tch_lwtsk_request(idle_tskid,&parm,FALSE);
		}
		tch_hal_enterSleepMode();
 	}
	tch_lwtsk_unregisterTask(idle_tskid);
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
		tch_hal_enableSystick(LSTICK_PERIOD);
		break;
	}
}


DECLARE_THREADROUTINE(__main){
	while(TRUE) ctx->Thread->sleep(1);
	return tchOK;
}



