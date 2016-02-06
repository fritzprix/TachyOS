/*
 * spi_test.c
 *
 *  Created on: 2014. 11. 1.
 *      Author: innocentevil
 */

#include "tch.h"
#include "test.h"
#include "kernel/util/string.h"


tchStatus do_spi_test(tch_core_api_t* ctx){
	tch_spiHandle_t* spihandle = NULL;
	uaddr_t faddr = 0;
	spi_config_t spiCfg;
	uint32_t leakTestcnt = 1000;

	tchStatus result;

	spiCfg.Baudrate = SPI_BAUDRATE_LOW;
	spiCfg.ClkMode = SPI_CLKMODE_0;
	spiCfg.FrmFormat = SPI_FRM_FORMAT_8B;
	spiCfg.FrmOrient = SPI_FRM_ORI_LSBFIRST;
	spiCfg.Role = SPI_ROLE_MASTER;

	tch_hal_module_spi_t* spi = ctx->Module->request(MODULE_TYPE_SPI);

	do{
		spihandle = spi->allocSpi(ctx,tch_spi0,&spiCfg,tchWaitForever,ActOnSleep);
		spihandle->close(spihandle);
	}while(leakTestcnt--);


	spihandle = spi->allocSpi(ctx,tch_spi0,&spiCfg,tchWaitForever,ActOnSleep);
	leakTestcnt = 0;

	const char* str = "Hello World!! This is SPI";
	int len = strlen(str);
	result = spihandle->write(spihandle,str,len);
	return result;
}
