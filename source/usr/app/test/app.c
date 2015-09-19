/*
 * app.c
 *
 *  Created on: 2015. 9. 10.
 *      Author: innocentevil
 */

#include "tch.h"
#include "thread_test.h"

int main(const tch* ctx){
	thread_performTest(ctx);
	while(TRUE){
		ctx->Thread->sleep(1);
	}
}


