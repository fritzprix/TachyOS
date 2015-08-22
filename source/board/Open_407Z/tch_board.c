/*
 * tch_board.c
 *
 *  Created on: 2014. 12. 9.
 *      Author: innocentevil
 */

#include "tch_board.h"
#include "tch_fs.h"



const static struct tch_board_descriptor_s bd_Descriptor = {
		.b_name = "Open_407Z",
		.b_major = 0,
		.b_minor = 0,
		.b_feature = 0
};


static int stdio_open(struct tch_file* filp);
static size_t stdio_read(struct tch_file* filp,char* rdata,size_t len);
static size_t stdio_write(struct tch_file* filp,const char* wdata,size_t len);
static int stdio_close(struct tch_file* filp);
static off_t stdio_seek(struct tch_file* filp,off_t offset,int whence);

static struct tch_file_operations stderr_fops = {

};


static struct tch_file_operations stdio_fops = {

};


const static struct tch_board_param BOARD_PARM = {
		.desc = &bd_Descriptor,
		.default_stdiofile = &stdio_fops,
		.default_errfile = &stderr_fops
};

static tch_usartHandle stdio_handle;


tch_boardParam tch_boardInit(const tch* ctx){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;
	stdio_handle = ctx->Device->usart->allocate(ctx,tch_USART1,&ucfg,tchWaitForever,ActOnSleep);

	return  &BOARD_PARM;
}

tchStatus tch_boardDeinit(const tch* env){

}


static int stdio_open(struct tch_file* filp){

}

static size_t stdio_read(struct tch_file* filp,char* rdata,size_t len){

}

static size_t stdio_write(struct tch_file* filp,const char* wdata,size_t len){

}

static int stdio_close(struct tch_file* filp){

}

static off_t stdio_seek(struct tch_file* filp,off_t offset,int whence){

}
