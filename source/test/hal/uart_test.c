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
	ucfg.Parity = env->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = env->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;

	tch_UartHandle* serial = NULL;


	uint8_t* printerThreadStk = (uint8_t*)env->Mem->alloc(1 << 10);
	tch_threadCfg thcfg;
	thcfg._t_name = "Printer";
	thcfg._t_routine = printerThreadRoutine;
	thcfg._t_stack = printerThreadStk;
	thcfg.t_stackSize = (1 << 10);
	thcfg.t_proior = Normal;

	tch_threadId printer = env->Thread->create(&thcfg,serial);
	env->Thread->start(printer);

	mcnt = 1000;
	const char* openMsg = "UART Opened By Main Thread \n\r";
	const char* myname = "I'm Main Thread \n\r";
	int size = env->uStdLib->string->strlen(myname);

	while(mcnt--){
		serial = env->Device->usart->open(env,&ucfg,osWaitForever,ActOnSleep);
		serial->write(serial,openMsg,env->uStdLib->string->strlen(openMsg));
		env->Thread->sleep(0);
		serial->write(serial,myname,size);
		env->Thread->sleep(0);
		env->Device->usart->close(serial);
	}

	mcnt++;
	if(env->Thread->join(printer,osWaitForever) != osOK)
		return osErrorOS;
	env->Device->usart->close(serial);
	return osOK;
}


static DECLARE_THREADROUTINE(printerThreadRoutine){
	pcnt = 1000;
	const char* openedMsg = "UART Opened by Printer \r\n";
	const char* myname = "I'm Printer Thread \r\n";
	int size = sys->uStdLib->string->strlen(myname);
	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = sys->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = sys->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;
	tch_UartHandle* serial = NULL;
	while(pcnt--){
		serial = sys->Device->usart->open(sys,&ucfg,osWaitForever,ActOnSleep);
		serial->write(serial,openedMsg,sys->uStdLib->string->strlen(openedMsg));
		sys->Thread->sleep(0);
		serial->write(serial,myname,size);
		sys->Thread->sleep(0);
		sys->Device->usart->close(serial);
	}
	return osOK;
}

