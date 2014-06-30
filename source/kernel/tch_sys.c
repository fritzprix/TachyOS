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
#include "kernel/tch_sched.h"





static tch_kernel_instance tch_sys_instance;



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


	portix->_enableISR();                   // interrupt enable
	tch_schedInit(&tch_sys_instance);

}

void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2){
	tch_exc_stack* sp = (tch_exc_stack*)_port_getThreadSP();
	tch_port_ix* tch_port = tch_sys_instance.tch_port;
	switch(sv_id){
	case SV_EXIT_FROM_SV:
		sp++;
		_port_setThreadSP((uint32_t)sp);
		tch_port->_kernel_unlock();
		return;
	case SV_THREAD_START:             // thread pointer arg1
		tch_schedStartThread((tch_thread_id) arg1);
		return;
	case SV_THREAD_SLEEP:
		tch_schedScheduleToSuspend(arg1);    ///< put current thread in the pend queue and update timeout value in the thread header
		return;
	case SV_THREAD_JOIN:
		return;
	}
}




void tch_error_handler(BOOL dump,osStatus status){

	/**
	 *  optional dump register
	 */
	while(1){
		asm volatile("NOP");
	}
}



