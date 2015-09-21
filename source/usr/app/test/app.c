/*
 * app.c
 *
 *  Created on: 2015. 9. 10.
 *      Author: innocentevil
 */

#include "tch.h"
#include "thread_test.h"
#include "mailq_test.h"

int main(const tch* ctx){
	mstat init_mstat;
	mstat fin_mstat;

//	ctx->Thread->yield(100);

	kmstat(&init_mstat);


	barrier_performTest((tch*) ctx);
	thread_performTest((tch*) ctx);
	mailq_performTest((tch*) ctx);

	monitor_performTest((tch*) ctx);
	semaphore_performTest((tch*) ctx);
	kmstat(&fin_mstat);



	while(TRUE){
		ctx->Thread->sleep(1);
	}
}


