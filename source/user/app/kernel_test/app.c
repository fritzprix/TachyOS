/*
 * app.c
 *
 *  Created on: 2015. 9. 10.
 *      Author: innocentevil
 */

#include "tch.h"
#include "thread_test.h"
#include "mailq_test.h"

int main(const tch_core_api_t* ctx){
	tchStatus testResult = tchOK;
	if((testResult = barrier_performTest((tch_core_api_t*) ctx)) != tchOK)
		return testResult;

	if((testResult = thread_performTest((tch_core_api_t*) ctx)) != tchOK)
		return testResult;

	if((testResult = mailq_performTest((tch_core_api_t*) ctx)) != tchOK)
		return testResult;

	if((testResult = monitor_performTest((tch_core_api_t*) ctx)) != tchOK)
		return testResult;

	if((testResult = semaphore_performTest((tch_core_api_t*) ctx)) != tchOK)
		return testResult;

	while(TRUE){
		ctx->Thread->sleep(1);
	}
}


