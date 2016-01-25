/*
 * spi_test.c
 *
 *  Created on: 2014. 11. 1.
 *      Author: innocentevil
 */

#include "tch.h"
#include "spi_test.h"

tchStatus spi_performTest(tch_core_api_t* ctx){
	tch_spiHandle* spihandle = NULL;
	uaddr_t faddr = 0;
	spi_config_t spiCfg;
	uint32_t leakTestcnt = 1000;



	spiCfg.Baudrate = SPI_BAUDRATE_LOW;
	spiCfg.ClkMode = SPI_CLKMODE_0;
	spiCfg.FrmFormat = SPI_FRM_FORMAT_8B;
	spiCfg.FrmOrient = SPI_FRM_ORI_LSBFIRST;
	spiCfg.Role = SPI_ROLE_MASTER;


	do{
		spihandle = ctx->Device->spi->allocSpi(ctx,tch_spi0,&spiCfg,tchWaitForever,ActOnSleep);
		spihandle->close(spihandle);
	}while(leakTestcnt--);



	spihandle = ctx->Device->spi->allocSpi(ctx,tch_spi0,&spiCfg,tchWaitForever,ActOnSleep);
	leakTestcnt = 0;

	const char* str = "Hello World!! This is SPI";
	int len = ctx->uStdLib->string->strlen(str);
	while(1){
		spihandle->write(spihandle,str,len);
	}
	return tchOK;

}
