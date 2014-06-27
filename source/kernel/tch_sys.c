/*
 * tch_sys.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 *
 *
 *
 *      this module contains most essential part of tch kernel.
 *
 */


#include "kernel/tch_kernel.h"





#ifndef MAIN_STACK_SIZE
#define MAIN_STACK_SIZE                     (1 << 10)
#endif

#ifndef IDLE_STACK_SIZE
#define IDLE_STACK_SIZE                     (1 << 8)
#endif


extern void* main(void* arg);
static void* idle(void* arg);


static tch_prototype tch_sys_instance;

static tch_thread_id MainThread_id;
static uint32_t MAIN_STACK[MAIN_STACK_SIZE];


static tch_thread_id IdleThread_id;
static uint32_t IDLE_STACK[IDLE_STACK_SIZE];




/***
 *  Initialize Kernel including...
 *  - initailize device driver and bind HAL interface
 *  - initialize architecture dependent part and bind port interface
 *  - bind User APIs to API type
 *  - initialize Idle thread
 */
void tch_kernelInit(void* arg){


	/**
	 *  dynamic binding of dependecy
	 */
	tch_sys_instance.tch_api.Hal = tch_hal_init();
	if(!tch_sys_instance.tch_api.Hal)
		tch_error_handler(false,osErrorValue);


	tch_sys_instance.tch_port = (tch_port_ix*)tch_port_init();
	if(!tch_sys_instance.tch_port)
		tch_error_handler(false,osErrorValue);
	tch_port_ix* portix = tch_sys_instance.tch_port;

	portix->_kernel_lock();

	tch* api = &tch_sys_instance.tch_api;
	api->Thread = Thread;


	tch_thread_cfg thcfg;
	thcfg._t_routine = main;
	thcfg._t_stack = MAIN_STACK;
	thcfg.t_stackSize = MAIN_STACK_SIZE;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";
	MainThread_id = Thread->create((tch*)&tch_sys_instance,&thcfg,&tch_sys_instance);

	thcfg._t_routine = idle;
	thcfg._t_stack = IDLE_STACK;
	thcfg.t_stackSize = IDLE_STACK_SIZE;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	IdleThread_id = Thread->create((tch*)&tch_sys_instance,&thcfg,&tch_sys_instance);

	portix->_enableISR();                   // interrupt enable
	tch_schedInit(&tch_sys_instance);

	/**
	 * start idle thread
	 */
	tch_sys_instance.tch_api.Thread->start(&tch_sys_instance,IdleThread_id);
}

void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_exc_stack* sp = NULL;
	tch_port_ix* tch_port = tch_sys_instance.tch_port;
	switch(sv_id){
	case SV_RETURN_TO_THREAD:
		sp++;
		tch_port->_kernel_unlock();
		return;
	case SV_THREAD_START:             // thread pointer arg1
		// compare current thread to new one
		tch_port->_jmpToKernelModeThread(tch_port->_restoreContext,arg1,arg2);
		return;
	}
}




void tch_error_handler(BOOL dump,osStatus status){

	/**
	 *  optional dump register
	 */
	while(1){
		asm volatile("nop");
	}
}



void* idle(void* arg){
	/**
	 * idle init
	 * - idle indicator init
	 */
	while(true){
		__DMB();
		__ISB();
		__WFI();
		__DMB();
		__ISB();
	}
}
