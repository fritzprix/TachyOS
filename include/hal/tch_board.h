/*
 * tch_board.h
 *
 *  Created on: 2014. 12. 9.
 *      Author: innocentevil
 */

#ifndef TCH_BOARD_H_
#define TCH_BOARD_H_


#include "tch.h"
#include "tch_fs.h"
#include "tch_halcfg.h"
#include "tch_haldesc.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct tch_board_descriptor_s {
	const char* 		b_name;		// board name in c string
	const char*			b_vname;	// vendor name in c string
	long int			b_pdata;	// production time in epoch time
	uint32_t			b_major;	// board major version
	uint32_t			b_minor;	// board minor version
	file*				b_logfile;	// io interface for kernel to print log
}* tch_board_descriptor;


typedef struct tch_uart_bs tch_uart_bs_t;
typedef struct tch_timer_bs tch_timer_bs_t;
typedef struct tch_spi_bs tch_spi_bs_t;
typedef struct tch_iic_bs tch_iic_bs_t;
typedef struct tch_adc_bs tch_adc_bs_t;
typedef struct tch_adc_ch_bs tch_adc_channel_bs_t;
typedef struct tch_sdio_bs tch_sdio_bs_t;



/**
 *  ========  board specific hardware assignment ========
 *  board implementation should supply below data structures
 *  on which HAL components depend. all the structures are
 *  specific to particular platform SoC and must be provided
 *  by tch_haldesc.h header in hal sub-directory for each HAL
 *  implementation.
 *	======================================================
 */


extern const tch_uart_bs_t UART_BD_CFGs[MFEATURE_GPIO];
extern const tch_timer_bs_t TIMER_BD_CFGs[MFEATURE_TIMER];
extern const tch_spi_bs_t SPI_BD_CFGs[MFEATURE_SPI];
extern const tch_iic_bs_t IIC_BD_CFGs[MFEATURE_IIC];
extern const tch_adc_bs_t ADC_BD_CFGs[MFEATURE_ADC];
extern const tch_adc_channel_bs_t ADC_CH_BD_CFGs[MFEATURE_ADC_Ch];
extern const tch_sdio_bs_t SDIO_BD_CFGs[MFEATURE_SDIO];


extern tch_board_descriptor tch_board_init(const tch_core_api_t* ctx);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_BOARD_H_ */
