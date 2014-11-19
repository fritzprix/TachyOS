/*
 * spi_test.c
 *
 *  Created on: 2014. 11. 1.
 *      Author: innocentevil
 */

#include "tch.h"
#include "spi_test.h"

tchStatus spi_performTest(tch* env){
	tch_spiHandle* spihandle = NULL;
	uaddr_t faddr = 0;
	tch_spiCfg spiCfg;
	uint32_t leakTestcnt = 1000;



	spiCfg.Baudrate = env->Device->spi->Baudrate.Low;
	spiCfg.ClkMode = env->Device->spi->ClkMode.Mode0;
	spiCfg.FrmFormat = env->Device->spi->FrmFormat.Frame8B;
	spiCfg.FrmOrient = env->Device->spi->FrmOrient.LSBFirst;
	spiCfg.Role = env->Device->spi->Role.Master;


	do{
		spihandle = env->Device->spi->allocSpi(env,env->Device->spi->spi.spi0,&spiCfg,osWaitForever,ActOnSleep);
		spihandle->close(spihandle);
	}while(leakTestcnt--);



	spihandle = env->Device->spi->allocSpi(env,env->Device->spi->spi.spi0,&spiCfg,osWaitForever,ActOnSleep);
	leakTestcnt = 0;

	const char* str = "Hello World!! This is SPI";
	int len = env->uStdLib->string->strlen(str);
	while(1){
		spihandle->write(spihandle,str,len);
	}
	return osOK;

}
