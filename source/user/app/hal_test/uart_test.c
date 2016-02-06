/*
 * uart_test.c
 *
 *  Created on: 2014. 10. 5.
 *      Author: innocentevil
 */

#include "tch.h"
#include "test.h"
#include "kernel/util/string.h"


static DECLARE_THREADROUTINE(printerThreadRoutine);

static int mcnt;
static int pcnt;
tchStatus uart_performTest(tch_core_api_t* ctx){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;

	tch_hal_module_usart_t* uart = ctx->Module->request(MODULE_TYPE_UART);
	if(!uart)
		return tchErrorResource;

	tch_usartHandle serial = NULL;


	thread_config_t thcfg;

	ctx->Thread->initConfig(&thcfg,printerThreadRoutine,Normal,(1 << 10),0,"printer");
	tch_threadId printer = ctx->Thread->create(&thcfg,serial);
	ctx->Thread->start(printer);

	mcnt = 50;
	const char* openMsg = "UART Opened By Main Thread \n\r";
	const char* myname = "I'm Main Thread \n\r";
	int size = strlen(myname);

	while(mcnt--){
		serial = uart->allocate(ctx,tch_USART2,&ucfg,tchWaitForever,ActOnSleep);
		serial->write(serial,openMsg,strlen(openMsg));
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
	int size = strlen(myname);
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;
	tch_usartHandle serial = (tch_usartHandle) ctx->Thread->getArg();
	while(pcnt--){
		serial->write(serial,openedMsg,strlen(openedMsg));
		ctx->Thread->yield(0);
		serial->write(serial,myname,size);
		ctx->Thread->yield(0);
		serial->close(serial);
	}
	return tchOK;
}

