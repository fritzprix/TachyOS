/*
 * tch_init.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */


#include "tch.h"





static uint32_t MAIN_STACK[MAIN_STACK_SIZE];
static tch_thread_id MAIN_THREAD;

static uint32_t IDLE_STACK[IDLE_STACK_SIZE];
static tch_thread_id IDLE_THREAD;

BOOL tch_kernelInit(void* arg){

	/***
	 *
	 */

}

