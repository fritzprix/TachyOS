/*
 * test.h
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */

#ifndef TEST_H_
#define TEST_H_

#include "tch.h"

#if defined(__cplusplus)
extern "C" {
#endif


extern tchStatus do_gpio_test(const tch_core_api_t* api);
extern tchStatus do_spi_test(const tch_core_api_t* api);
extern tchStatus do_timer_test(const tch_core_api_t* api);
extern tchStatus do_uart_test(const tch_core_api_t* api);
extern tchStatus do_iic_test(const tch_core_api_t* api);
extern tchStatus do_sdio_test(const tch_core_api_t* api);



#if defined(__cplusplus)
}
#endif


#endif /* TEST_H_ */
