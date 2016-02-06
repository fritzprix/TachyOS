/*
 * main.c
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */


#include "tch.h"
#include "led_blinker.h"
#include "sdio_working.h"

DECLARE_THREADROUTINE(main)
{
	start_led_blink(ctx);
	start_sdio_test(ctx);
	while(TRUE)
	{
		ctx->Thread->sleep(1);
	}
	exit_led_blink(ctx);
}


