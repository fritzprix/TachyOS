/*
 * tch_init.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */


#include "tch.h"
#include "port/acm4f/tch_port.h"


static const tch_hal*     _TCH_HAL;
static const tch_port_ix* _TCH_PORT;

static uint32_t MAIN_STACK[MAIN_STACK_SIZE];
static tch_thread_id MAIN_THREAD;

static uint32_t IDLE_STACK[IDLE_STACK_SIZE];
static tch_thread_id IDLE_THREAD;

BOOL tch_kernelInit(void* arg){

	/***
	 *  Device init hal initailize
	 */

	/**
	 *  dynamic binding of dependecy
	 */
	_TCH_HAL = tch_hal_init();
	_TCH_PORT = tch_port_init();

}

