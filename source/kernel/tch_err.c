/*
 * tch_err.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *  Created on: 2015. 7. 4.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "kernel/tch_err.h"

/**
 * exception detected by software(kernel)
 */
void tch_kernel_onSoftException(tch_threadId who, tchStatus err, const char* msg)
{
	while (TRUE)
	{
		__WFE();
	}
}

void tch_kernel_onPanic(const char* floc, int lno, const char* msg)
{
	while (TRUE)
	{
		__WFE();
	}
}

void tch_kernel_onHardException(int fault, int spec)
{
	while (TRUE)
	{
		__WFE();
	}
}
