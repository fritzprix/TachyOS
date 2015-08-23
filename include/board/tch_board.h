/*
 * tch_board.h
 *
 *  Created on: 2014. 12. 9.
 *      Author: innocentevil
 */

#ifndef TCH_BOARD_H_
#define TCH_BOARD_H_


#include "tch.h"
#include "tch_halcfg.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_board_descriptor_s {
	const char* 	b_name;		// board name in c string
	uint32_t		b_major;	// board major version
	uint32_t		b_minor;	// board minor version
	uint32_t		b_feature;	// features supported
}* tch_board_descriptor;

typedef struct tch_board_param {
	tch_board_descriptor 	desc;
	const struct tch_file*	default_stdiofile;
	const struct tch_file* 	default_errfile;
}* tch_boardParam;

extern tch_boardParam tch_boardInit(const tch* env);
extern tchStatus tch_boardDeinit(const tch* env);



extern __TCH_STATIC_INIT tch_uart_bs UART_BD_CFGs[MFEATURE_GPIO];
extern __TCH_STATIC_INIT tch_timer_bs TIMER_BD_CFGs[MFEATURE_TIMER];
extern __TCH_STATIC_INIT tch_spi_bs SPI_BD_CFGs[MFEATURE_SPI];
extern __TCH_STATIC_INIT tch_iic_bs IIC_BD_CFGs[MFEATURE_IIC];
extern __TCH_STATIC_INIT tch_adc_bs ADC_BD_CFGs[MFEATURE_ADC];
extern __TCH_STATIC_INIT tch_adc_channel_bs ADC_CH_BD_CFGs[MFEATURE_ADC_Ch];

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BOARD_H_ */
