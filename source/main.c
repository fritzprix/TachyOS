/*
 * main.c
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#include "main.h"
#include "tch.h"

void* main(void* arg) {
	tch* tch_api = (tch*)arg;
	tch_thread_ix* Thread = (tch_thread_ix*) tch_api->Thread;
	uint32_t val = 0;
	float hv = 0.1f;
	while (1) {
		val++;
		hv += 0.2f;
		Thread->sleep(500);
	}
	return 0;
}
