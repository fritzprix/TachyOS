/*
 * hal_utest.c
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */




#include "tch.h"
#include "test.h"


DECLARE_THREADROUTINE(main)
{
	tchStatus res;
	if((res = do_gpio_test(ctx)) != tchOK)
	{
		print_error(res, "gpio test failed!!\n\r");
	}
	if((res = do_iic_test(ctx)) != tchOK)
	{
		print_error(res, "i2c test failed!!\n\r");
	}
	if((res = do_uart_test(ctx)) != tchOK)
	{
		print_error(res, "uart test failed!!\n\r");
	}
	if((res = do_timer_test(ctx)) != tchOK)
	{
		print_error(res, "timer test failed!!\n\r");
	}
	if((res = do_sdio_test(ctx)) != tchOK)
	{
		print_error(res, "sdio test failed!!\n\r");
	}
	while(TRUE)
	{
		ctx->Thread->sleep(1);
	}
}
