/*
 * uart_test.c
 *
 *  Created on: 2014. 10. 5.
 *      Author: innocentevil
 */

#include "tch.h"
#include "uart_test.h"


static DECLARE_THREADROUTINE(printerThreadRoutine);

static int mcnt;
static int pcnt;
tchStatus uart_performTest(tch* ctx){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;

	tch_usartHandle serial = NULL;


	tch_threadCfg thcfg;

	ctx->Thread->initCfg(&thcfg,printerThreadRoutine,Normal,(1 << 10),0,"printer");
	tch_threadId printer = ctx->Thread->create(&thcfg,serial);
	ctx->Thread->start(printer);

	mcnt = 50;
	const char* openMsg = "UART Opened By Main Thread \n\r";
	const char* myname = "I'm Main Thread \n\r";
	int size = ctx->uStdLib->string->strlen(myname);

	while(mcnt--){
		serial = ctx->Device->usart->allocate(ctx,tch_USART2,&ucfg,tchWaitForever,ActOnSleep);
		serial->write(serial,openMsg,ctx->uStdLib->string->strlen(openMsg));
		ctx->Thread->yield(0);
		serial->write(serial,myname,size);
		ctx->Thread->yield(0);
		serial->close(serial);
	}

	if(ctx->Thread->join(printer,tchWaitForever) != tchOK)
		return tchErrorOS;
	serial->close(serial);
	return tchOK;
}


static DECLARE_THREADROUTINE(printerThreadRoutine){
	pcnt = 50;
	const char* openedMsg = "UART Opened by Printer \r\n";
	const char* myname = "I'm Printer Thread \r\n";
	int size = ctx->uStdLib->string->strlen(myname);
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;
	tch_usartHandle serial = NULL;
	while(pcnt--){
		serial = ctx->Device->usart->allocate(ctx,tch_USART2,&ucfg,tchWaitForever,ActOnSleep);
		serial->write(serial,openedMsg,ctx->uStdLib->string->strlen(openedMsg));
		ctx->Thread->yield(0);
		serial->write(serial,myname,size);
		ctx->Thread->yield(0);
		serial->close(serial);
	}
	return tchOK;
}

