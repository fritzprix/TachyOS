/*
 * tch_boardcfg.c
 *
 *  Created on: Aug 23, 2015
 *      Author: innocentevil
 */

#include "tch_board.h"
#include "kernel/tch_fs.h"


__TCH_STATIC_INIT tch_uart_bs_t UART_BD_CFGs[MFEATURE_GPIO] = {
		{
				.txdma = DMA_Str15,
				.rxdma = DMA_NOT_USED,
				.txch = DMA_Ch4,
				.rxch = DMA_Ch4,
				.port = tch_gpio0,
				.txp = 9,
				.rxp = 10,
				.ctsp = 11,
				.rtsp = 12,
				.afv = 7
		},
		{
				.txdma = DMA_Str6,
				.rxdma = DMA_NOT_USED,
				.txch = DMA_Ch4,
				.rxch = DMA_Ch4,
				.port = tch_gpio0,
				.txp = 2,
				.rxp = 3,
				.ctsp = 0,
				.rtsp = 1,
				.afv = 7
		},
		{
				.txdma = DMA_Str4,
				.rxdma = DMA_NOT_USED,
				.txch = DMA_Ch7,
				.rxch = DMA_Ch4,
				.port = tch_gpio1,
				.txp = 10,
				.rxp = 11,
				.ctsp = 13,
				.rtsp = 14,
				.afv = 7
		},
		{
				.txdma = DMA_Str4,
				.rxdma = DMA_NOT_USED,
				.txch = DMA_Ch4,
				.rxch = DMA_Ch4,
				.port = tch_gpio0,
				.txp = 0,
				.rxp = 1,
				.rtsp = -1,
				.rtsp = -1,
				.afv = 8
		}
};

/**
 * 	gpIo_x         port;
	uint8_t        ch1p;
	uint8_t        ch2p;
	uint8_t        ch3p;
	uint8_t        ch4p;
	uint8_t        afv;
 */

__TCH_STATIC_INIT tch_timer_bs_t TIMER_BD_CFGs[MFEATURE_TIMER] = {
		{// TIM2
				.port = tch_gpio0,
				.chp = {
						0,
						1,
						2,
						3
				},
				.afv = 1
		},
		{// TIM3
				tch_gpio1,
				.chp = {
						4,
						5,
						0,
						1
				},
				.afv = 2
		},
		{// TIM4
				.port = tch_gpio1,
				.chp = {
						6,
						7,
						8,
						9
				},
				.afv = 2
		},
		{// TIM5
				.port = tch_gpio7,
				.chp = {
						10,
						11,
						12,
						-1
				},
				.afv = 2
		},
		{// TIM9
				.port = tch_gpio4,
				.chp = {
						5,
						6,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM10
				.port = tch_gpio1,
				.chp = {
						8,
						-1,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM11
				.port = tch_gpio1,
				.chp = {
						9,
						-1,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM12
				.port =  tch_gpio1,
				.chp = {
						14,
						15,
						-1,
						-1
				},
				.afv = 9
		},
		{// TIM13
				.port = tch_gpio5,
				.chp = {
						8,
						-1,
						-1,
						-1
				},
				.afv = 9
		},
		{// TIM14
				.port = tch_gpio5,
				.chp = {
						9,
						-1,
						-1,
						-1
				},
				.afv = 9
		}
};

/**
 * 	spi_t          spi;
	dma_t          txdma;
	dma_t          rxdma;
	gpIo_x         port;
	uint8_t        mosi;
	uint8_t        miso;
	uint8_t        sck;
 */

__TCH_STATIC_INIT tch_spi_bs_t SPI_BD_CFGs[MFEATURE_SPI] = {
		{

				.txdma = DMA_Str13,    //dma2_stream5
				.rxdma = DMA_Str10,    //dma2_stream2
				.txch = 3,            //dma channel 3
				.rxch = 3,            //dma channel 3
				.port = 0,            // port A (0)
				.mosi = 7,            // pin  7
				.miso = 6,            // pin  6
				.sck = 5,            // pin  5
				.afv = 5             // af5
		},
		{
				.txdma = DMA_Str4,     //dma1_stream4
				.rxdma = DMA_Str3,     //dma1_stream3
				.txch = 0,
				.rxch = 0,
				.port = 1,            // port B (1)
				.mosi = 15,           // pin  15
				.miso = 14,           // pin  14
				.sck = 13,           // pin  13
				.afv = 5             // af5
		},
		{
				.txdma = DMA_Str7,     //dma1_stream7
				.rxdma = DMA_Str2,     //dma1_stream2
				.txch = 0,
				.rxch = 0,
				.port = 2,            // port C (2)
				.mosi = 12,           // pin  12
				.miso = 11,           // pin  11
				.sck = 10,           // pin  10
				.afv = 6
		}
};
/**
 * 	dma_t         txdma;
	dma_t         rxdma;
	uint8_t       txch;
	uint8_t       rxch;
	gpIo_x        port;
	uint8_t       scl;
	uint8_t       sda;
	uint8_t       afv;
 */

__TCH_STATIC_INIT tch_iic_bs_t IIC_BD_CFGs[MFEATURE_IIC] = {
		{      // IIC 1
				.txdma = DMA_Str6,    // dma1_stream 6
				.rxdma = DMA_Str5,    // dma1_stream 5
				.txch = 1,           // dma channel 1
				.rxch = 1,           // dma channel 1
				.port = 1,           // port B(1)
				.scl = 6,           // pin 6
				.sda = 7,           // pin 7
				.afv = 4            // afv
		},
		{
				.txdma = DMA_Str7,   // dma1_stream 2
				.rxdma = DMA_Str2,   // dma1_stream 7
				.txch = 7,          // dma channel 7
				.rxch = 7,          // dma channel 7
				.port = 1,          // port B(1)
				.scl = 10,          // pin 10
				.sda = 11,          // pin 11
				.afv = 4           // afv
		},
		{
				.txdma = DMA_Str2,   // dma1 stream 2
				.rxdma = DMA_Str4,   // dma1 stream 4
				.txch = 3,          // dma channel 3
				.rxch = 3,          // dma channel 3
				.port = 7,          // port H(7)
				.scl = 7,          // pin 7
				.sda = 8,          // pin 8
				.afv = 4           // afv
		}
};


/**
 * 	dma_t         dma;
	uint8_t       dmaCh;
	tch_timer     timer;
	uint8_t       timerCh;
	uint8_t       timerExtsel;
 */
__TCH_STATIC_INIT tch_adc_bs_t ADC_BD_CFGs[MFEATURE_ADC] = {
		{
				.dma = DMA_Str12,
				.dmaCh = DMA_Ch0,
				.timer =  tch_TIMER0,  // TIM2
				.timerCh = 2,         // CH2
				.timerExtsel = 3      // Timer 2 CC2 Event
		},
		{
				.dma = DMA_Str10,
				.dmaCh = DMA_Ch1,
				.timer = tch_TIMER1,
				.timerCh = 1,
				.timerExtsel = 7     // Timer 3 CC1 Event
		},
		{
				.dma = DMA_Str8,
				.dmaCh = DMA_Ch0,
				.timer = tch_TIMER2,
				.timerCh = 4,
				.timerExtsel = 9     // Timer 4 CC4 Event
		}
};


/**
 * 	tch_port      port;
	uint8_t       adc_routemsk;
	uint8_t       occp;
 */
__TCH_STATIC_INIT tch_adc_channel_bs_t ADC_CH_BD_CFGs[MFEATURE_ADC_Ch] = {
		{
				.port = {tch_gpio0,0},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch0
		{
				.port = {tch_gpio0,1},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch1
		{
				.port = {tch_gpio0,2},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch2
		{
				.port = {tch_gpio0,3},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch3
		{
				.port = {tch_gpio0,4},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch4
		{
				.port = {tch_gpio0,5},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch5
		{
				.port = {tch_gpio0,6},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch6
		{
				.port = {tch_gpio0,7},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch7
		{
				.port = {tch_gpio1,0},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch8
		{
				.port = {tch_gpio1,1},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},// ch9
		{
				.port = {tch_gpio2,0},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch10
		{
				.port = {tch_gpio2,1},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch11
		{
				.port = {tch_gpio2,2},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch12
		{
				.port = {tch_gpio2,3},
				.adc_routemsk  = (ADC1_Bit | ADC2_Bit | ADC3_Bit),
				.occp = 0
		},// ch13
		{
				.port = {tch_gpio2,4},
				.adc_routemsk = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		},
		{
				.port = {tch_gpio2,5},
				.adc_routemsk  = (ADC1_Bit | ADC2_Bit),
				.occp = 0
		}
};

struct tch_board_descriptor_s BOARD_DESCRIPTOR =
{
		.b_name = "Open_407Z",
		.b_major = 1,
		.b_minor = 0,
		.b_vname = "wavetech",
		.b_pdata = 0l
};

static int log_open(struct tch_file* filp);
static ssize_t log_read(struct tch_file* filp, char* bp,size_t len);
static ssize_t log_write(struct tch_file* filp, const char* bp, size_t len);
int  log_close(struct tch_file* filp);
ssize_t log_seek(struct tch_file* filp, size_t offset, int whence);

file_operations_t LOG_FILE = {
		.open = log_open,
		.read = log_read,
		.write = log_write,
		.close = log_close,
		.seek = log_seek
};


tch_board_descriptor tch_board_init(const tch* ctx)
{
	BOARD_DESCRIPTOR.b_logfile = &LOG_FILE;
	return &BOARD_DESCRIPTOR;
}


static int log_open(struct tch_file* filp)
{

}

static ssize_t log_read(struct tch_file* filp, char* bp,size_t len)
{

}

static ssize_t log_write(struct tch_file* filp, const char* bp, size_t len)
{

}

int  log_close(struct tch_file* filp)
{

}

ssize_t log_seek(struct tch_file* filp, size_t offset, int whence)
{

}



