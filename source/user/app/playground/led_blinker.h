/*
 * led_blinker.h
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */

#ifndef LED_BLINKER_H_
#define LED_BLINKER_H_

#if defined(__cplusplus)
extern "C" {
#endif


extern tchStatus start_led_blink(const tch_core_api_t* api);
extern tchStatus exit_led_blink(const tch_core_api_t* api);


#if defined(__cplusplus)
}
#endif


#endif /* LED_BLINKER_H_ */
