/*
 * tch_board.c
 *
 *  Created on: 2014. 12. 9.
 *      Author: innocentevil
 */

#include "tch_board.h"




static int tch_bdstd_write(const uint8_t* buf,uint32_t sz);
static int tch_bdstd_read(uint8_t* buf,uint32_t sz);
static void tch_bdstd_close();


static int tch_bderr_write(const uint8_t* buf,uint32_t sz);
static int tch_bderr_read(uint8_t* buf,uint32_t sz);
static void tch_bderr_close();

const static struct tch_board_descriptor_s bd_Descriptor = {
		.b_name = "Open_407Z",
		.b_major = 0,
		.b_minor = 0,
		.b_feature = 0
};

const static struct tch_mfile_s tch_stdioFile = {
		.close = tch_bdstd_close,
		.read = tch_bdstd_read,
		.write = tch_bdstd_write
};

const static struct tch_mfile_s tch_errFile = {
		.close = tch_bderr_close,
		.read = tch_bderr_read,
		.write = tch_bderr_write
};

const static struct tch_board_handle_s BOARD_HANDLE = {
		.bd_desc = &bd_Descriptor,
		.bd_stdio = &tch_stdioFile,
		.bd_errio = &tch_errFile
};

static tch_usartHandle stdio_handle;

tch_boardHandle tch_boardInit(const tch* ctx){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;
	stdio_handle = ctx->Device->usart->allocate(ctx,tch_USART1,&ucfg,tchWaitForever,ActOnSleep);

	return &BOARD_HANDLE;
}

tchStatus tch_boardDeinit(const tch* env){

}


static int tch_bdstd_write(const uint8_t* buf,uint32_t sz){
	if(stdio_handle == NULL)
		return FALSE;
	return (stdio_handle->write(stdio_handle,buf,sz) == tchOK);
}

static int tch_bdstd_read(uint8_t* buf,uint32_t sz){
	if(stdio_handle == NULL)
		return 0;
	return stdio_handle->read(stdio_handle,buf,sz,100);
}

static void tch_bdstd_close(){
	if(stdio_handle == NULL)
		return;
	stdio_handle->close(stdio_handle);
}

static int tch_bderr_write(const uint8_t* buf,uint32_t sz){

}

static int tch_bderr_read(uint8_t* buf,uint32_t sz){

}

static void tch_bderr_close(){

}
