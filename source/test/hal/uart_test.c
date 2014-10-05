/*
 * uart_test.c
 *
 *  Created on: 2014. 10. 5.
 *      Author: innocentevil
 */

#include "tch.h"
#include "uart_test.h"


static DECLARE_THREADROUTINE(printerThreadRoutine);


tchStatus uart_performTest(tch* env){

	tch_UartCfg ucfg;
	ucfg.Buadrate = 115200;
	ucfg.FlowCtrl = FALSE;
	ucfg.Parity = env->Device->usart->Parity.Parity_Non;
	ucfg.StopBit = env->Device->usart->StopBit.StopBit1B;
	ucfg.UartCh = 2;
	tch_UartHandle* serial = env->Device->usart->open(env,&ucfg,osWaitForever,ActOnSleep);


	uint8_t* printerThreadStk = (uint8_t*)env->Mem->alloc(1 << 9);
	tch_threadCfg thcfg;
	thcfg._t_name = "Printer";
	thcfg._t_routine = printerThreadRoutine;
	thcfg._t_stack = printerThreadStk;
	thcfg.t_stackSize = (1 << 9);
	thcfg.t_proior = Normal;

	tch_threadId printer = env->Thread->create(&thcfg,serial);
	env->Thread->start(printer);

	uint32_t cnt = 100;
	const char* myname = "I'm Main Thread";
	int size = env->uStdLib->string->strlen(myname);
	while(cnt--){
		serial->write(serial,myname,size);
	}

	if(env->Thread->join(printer,osWaitForever) != osOK)
		return osErrorOS;

	if(env->Device->usart->close(serial))
		return osOK;
	return osErrorOS;
}


static DECLARE_THREADROUTINE(printerThreadRoutine){
	tch_UartHandle* serial = (tch_UartHandle*) sys->Thread->getArg();
	uint32_t cnt = 100;
	const char* myname = "I'm Printer Thread";
	int size = sys->uStdLib->string->strlen(myname);
	while(cnt--){
		serial->write(serial,myname,size);
	}
	return osOK;
}

