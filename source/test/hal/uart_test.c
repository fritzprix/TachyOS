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
tchStatus uart_performTest(tch* env){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;

	tch_usartHandle serial = NULL;


	tch_threadCfg thcfg;
	thcfg._t_name = "Printer";
	thcfg._t_routine = printerThreadRoutine;
	thcfg.t_stackSize = (1 << 10);
	thcfg.t_proior = Normal;

	tch_threadId printer = env->Thread->create(&thcfg,serial);
	env->Thread->start(printer);

	mcnt = 50;
	const char* openMsg = "UART Opened By Main Thread \n\r";
	const char* myname = "I'm Main Thread \n\r";
	int size = env->uStdLib->string->strlen(myname);

	while(mcnt--){
		serial = env->Device->usart->allocate(env,tch_USART2,&ucfg,tchWaitForever,ActOnSleep);
		serial->write(serial,openMsg,env->uStdLib->string->strlen(openMsg));
		env->Thread->yield(0);
		serial->write(serial,myname,size);
		env->Thread->yield(0);
		serial->close(serial);
	}

	if(env->Thread->join(printer,tchWaitForever) != tchOK)
		return tchErrorOS;
	serial->close(serial);
	return tchOK;
}


static DECLARE_THREADROUTINE(printerThreadRoutine){
	pcnt = 50;
	const char* openedMsg = "UART Opened by Printer \r\n";
	const char* myname = "I'm Printer Thread \r\n";
	int size = env->uStdLib->string->strlen(myname);
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = USART_Parity_NON;
	ucfg.StopBit = USART_StopBit_1B;
	tch_usartHandle serial = NULL;
	while(pcnt--){
		serial = env->Device->usart->allocate(env,tch_USART2,&ucfg,tchWaitForever,ActOnSleep);
		serial->write(serial,openedMsg,env->uStdLib->string->strlen(openedMsg));
		env->Thread->yield(0);
		serial->write(serial,myname,size);
		env->Thread->yield(0);
		serial->close(serial);
	}
	return tchOK;
}

