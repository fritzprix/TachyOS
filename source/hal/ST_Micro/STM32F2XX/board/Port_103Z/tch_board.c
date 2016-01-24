/*
 * tch_boardcfg.c
 *
 *  Created on: Aug 23, 2015
 *      Author: innocentevil
 */

#include "tch_board.h"


__TCH_STATIC_INIT tch_uart_bs_t UART_BD_CFGs[MFEATURE_GPIO] = {
		{
				.txdma = DMA_NOT_USED,
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
				.ctsp = -1,
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
				{
						0,
						1,
						2,
						3
				},
				.afv = 1
		},
		{// TIM3
				.port = tch_gpio1,
				{
						4,
						5,
						0,
						1
				},
				.afv = 2
		},
		{// TIM4
				.port = tch_gpio1,
				{
						6,
						7,
						8,
						9
				},
				.afv = 2
		},
		{// TIM5
				.port = tch_gpio7,
				{
						10,
						11,
						12,
						-1
				},
				.afv = 2
		},
		{// TIM9
				.port = tch_gpio4,
				{
						5,
						6,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM10
				.port = tch_gpio1,
				{
						8,
						-1,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM11
				.port = tch_gpio1,
				{
						9,
						-1,
						-1,
						-1
				},
				.afv = 3
		},
		{// TIM12
				.port = tch_gpio1,
				{
						14,
						15,
						-1,
						-1
				},
				.afv = 9
		},
		{// TIM13
				.port = tch_gpio5,
				{
						8,
						-1,
						-1,
						-1
				},
				.afv = 9
		},
		{// TIM14
				.port =tch_gpio5,
				{
						9,
						-1,
						-1,
						-1
				},
				.afv = 9
		}
};


__TCH_STATIC_INIT tch_spi_bs_t SPI_BD_CFGs[MFEATURE_SPI] = {
		{

				.txdma = DMA_NOT_USED,
				.rxdma = DMA_NOT_USED,
				.txch = 3,
				.rxch = 3,
				.port = 0,            // port A (0)
				.mosi = 7,            // pin  7
				.miso = 6,            // pin  6
				.sck = 5,             // pin  5
				.afv = 5              // af5
		},
		{
				.txdma = DMA_Str4,     //dma1_stream4
				.rxdma = DMA_Str3,     //dma1_stream3
				.txch = 0,
				.rxch = 0,
				.port = 1,             // port B (1)
				.mosi = 15,            // pin  15
				.miso = 14,            // pin  14
				.sck = 13,             // pin  13
				.afv = 5               // af5
		},
		{
				.txdma = DMA_Str7,     //dma1_stream7
				.rxdma = DMA_Str2,     //dma1_stream2
				.txch = 0,
				.rxch = 0,
				.port = 2,             // port C (2)
				.mosi = 12,            // pin  12
				.miso = 11,            // pin  11
				.sck = 10,             // pin  10
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
				DMA_Str6,    // dma1_stream 6
				DMA_Str5,    // dma1_stream 5
				1,           // dma channel 1
				1,           // dma channel 1
				1,           // port B(1)
				6,           // pin 6
				7,           // pin 7
				4            // afv
		},
		{
				DMA_Str7,   // dma1_stream 2
				DMA_Str2,   // dma1_stream 7
				7,          // dma channel 7
				7,          // dma channel 7
				1,          // port B(1)
				10,          // pin 10
				11,          // pin 11
				4           // afv
		},
		{
				DMA_Str2,   // dma1 stream 2
				DMA_Str4,   // dma1 stream 4
				3,          // dma channel 3
				3,          // dma channel 3
				7,          // port H(7)
				7,          // pin 7
				8,          // pin 8
				4           // afv
		}
};

/**
 * typedef struct _tch_adc_bs_t {
	dma_t         dma;
	uint8_t       dmach;
	uint8_t       afv;
}tch_adc_bs;

typedef struct _tch_adc_com_bs_t{
	tch_port ports[MFEATURE_ADC_Ch];
	uint32_t occp_status;
};
 */


__TCH_STATIC_INIT tch_adc_bs_t ADC_BD_CFGs[MFEATURE_ADC] = {
		{
				DMA_Str12,
				DMA_Ch0,
				tch_TIMER0,  // TIM2
				2,         // CH2
				3      // Timer 2 CC2 Event
		},
		{
				DMA_Str10,
				DMA_Ch1,
				tch_TIMER1,
				1,
				7     // Timer 3 CC1 Event
		},
		{
				DMA_Str8,
				DMA_Ch0,
				tch_TIMER2,
				4,
				9     // Timer 4 CC4 Event
		}
};

/**
 *
 * 	tch_port      port;
	uint8_t       adc_routemsk;
	uint8_t       occp;
 */
__TCH_STATIC_INIT tch_adc_channel_bs_t ADC_CH_BD_CFGs[MFEATURE_ADC_Ch] = {
		{{tch_gpio0,0},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch0
		{{tch_gpio0,1},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch1
		{{tch_gpio0,2},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch2
		{{tch_gpio0,3},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch3
		{{tch_gpio0,4},(ADC1_Bit | ADC2_Bit),0},// ch4
		{{tch_gpio0,5},(ADC1_Bit | ADC2_Bit),0},// ch5
		{{tch_gpio0,6},(ADC1_Bit | ADC2_Bit),0},// ch6
		{{tch_gpio0,7},(ADC1_Bit | ADC2_Bit),0},// ch7
		{{tch_gpio1,0},(ADC1_Bit | ADC2_Bit),0},// ch8
		{{tch_gpio1,1},(ADC1_Bit | ADC2_Bit),0},// ch9
		{{tch_gpio2,0},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch10
		{{tch_gpio2,1},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch11
		{{tch_gpio2,2},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch12
		{{tch_gpio2,3},(ADC1_Bit | ADC2_Bit | ADC3_Bit),0},// ch13
		{{tch_gpio2,4},(ADC1_Bit | ADC2_Bit),0},
		{{tch_gpio2,5},(ADC1_Bit | ADC2_Bit),0}
};



struct tch_board_descriptor_s BOARD_DESCRIPTOR =
{
		.b_name = "Port_103Z",
		.b_major = 1,
		.b_minor = 0,
		.b_vname = "wavetech",
		.b_pdata = 0l
};


static int log_open(struct tch_file* filp);
static ssize_t log_read(struct tch_file* filp, char* bp,size_t len);
static ssize_t log_write(struct tch_file* filp, const char* bp, size_t len);
static int  log_close(struct tch_file* filp);
static ssize_t log_seek(struct tch_file* filp, size_t offset, int whence);

static tch* context;
static tch_usartHandle log_serial;

file_operations_t LOG_IO = {
		.open = log_open,
		.read = log_read,
		.write = log_write,
		.close = log_close,
		.seek = log_seek
};



tch_board_descriptor tch_board_init(const tch* ctx)
{
	context = (tch*) ctx;
	BOARD_DESCRIPTOR.b_logfile = &LOG_IO;
	context = NULL;
	log_serial = NULL;
	return &BOARD_DESCRIPTOR;
}


static int log_open(struct tch_file* filp)
{
	tch_device_service_usart* uart = context->Service->request(MODULE_TYPE_UART);
	tch_UartCfg uart_config;
	uart_config.Buadrate = 115200;
	uart_config.FlowCtrl = FALSE;
	uart_config.Parity = USART_Parity_NON;
	uart_config.StopBit = USART_StopBit_1B;

	//tch_usartHandle (*const allocate)(const tch* env,uart_t port,tch_UartCfg* cfg,uint32_t timeout,tch_PwrOpt popt);
	log_serial = uart->allocate(context, tch_USART2,&uart_config,tchWaitForever,ActOnSleep);
	if(!log_serial)
		return FALSE;
	return TRUE;

}

static ssize_t log_read(struct tch_file* filp, char* bp,size_t len)
{
	if(!log_serial)
		return tchErrorResource;
	return log_serial->read(log_serial,bp,len,tchWaitForever);
}

static ssize_t log_write(struct tch_file* filp, const char* bp, size_t len)
{
	if(!log_serial)
		return tchErrorResource;
	return log_serial->write(log_serial,bp,len);
}

int  log_close(struct tch_file* filp)
{
	if(!log_serial)
		return tchErrorResource;
	return log_serial->close(log_serial) == tchOK? TRUE : FALSE;
}

ssize_t log_seek(struct tch_file* filp, size_t offset, int whence)
{
	return 0;
}

