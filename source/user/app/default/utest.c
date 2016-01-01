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
	do_test_thread(ctx);
	return tchOK;
}
