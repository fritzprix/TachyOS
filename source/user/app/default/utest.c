/*
 * utest.c
 *
 *  Created on: 2015. 12. 31.
 *      Author: hyunbok
 */


#include "tch.h"
#include "utest.h"



DECLARE_THREADROUTINE(main)
{
	tchStatus res;
	if((res = do_thread_test(ctx)) != tchOK)
	{
		print_error(res, "thread test failed %d\n\r",res);
	}

	if((res = do_mailq_test(ctx)) != tchOK)
	{
		print_error(res, "mailq test failed %d\n\r",res);
	}

	while(TRUE)
	{
		ctx->Thread->sleep(1);
	}
	return tchOK;
}
