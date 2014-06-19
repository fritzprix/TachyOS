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


#include "tch.h"
#include "port/acm4f/tch_port.h"






#define SV_RETURN_TO_THREAD                 ((uint8_t) )

extern void* main(void* arg);
static void* idle(void* arg);


static const tch_hal*     _tch_hal;
static const tch_port_ix* _tch_port;
static const tch TCH_API = {
		Thread,
		Sig,
		Condv,
		Mtx,
		Sem,
		MsgQ,
		MailQ,
		Mempool,
		Hal
};

static tch_thread_id MainThread_id;
static uint32_t MAIN_STACK[MAIN_STACK_SIZE];
static tch_thread_id MAIN_THREAD;


static tch_thread_id IdleThread_id;
static uint32_t IDLE_STACK[IDLE_STACK_SIZE];
static tch_thread_id IDLE_THREAD;





BOOL tch_kernelInit(void* arg){

	/***
	 *  Device init hal initailize
	 */

	/**
	 *  dynamic binding of dependecy
	 */
	_tch_hal = tch_hal_init();
	if(!_tch_hal)
		tch_error_handler(false,osErrorValue);


	_tch_port = tch_port_init();
	if(!_tch_port)
		tch_error_handler(false,osErrorValue);
	_tch_port->_kernel_lock();



	tch_thread_cfg thcfg;
	thcfg._t_routine = main;
	thcfg._t_stack = MAIN_STACK;
	thcfg.t_stackSize = MAIN_STACK_SIZE;
	thcfg.t_proior = Normal;
	thcfg._t_name = "main";
	MainThread_id = Thread->create(&thcfg,&TCH_API);

	tch_thread_cfg thcfg;
	thcfg._t_routine = idle;
	thcfg._t_stack = IDLE_STACK;
	thcfg.t_stackSize = IDLE_STACK_SIZE;
	thcfg.t_proior = Idle;
	thcfg._t_name = "idle";
	IdleThread_id = Thread->create(&thcfg,&TCH_API);
}

void tch_kernelSvCall(uint32_t sv_id,void* arg1, void* arg2){
	arm_exc_stack* sp = NULL;
	switch(sv_id){
	case SV_RETURN_TO_THREAD:
		sp++;
		_tch_port->_kernel_unlock();
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
