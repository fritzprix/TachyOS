/*
 * sdio_working.c
 *
 *  Created on: 2016. 1. 30.
 *      Author: innocentevil
 */

#ifndef SDIO_WORKING_C_
#define SDIO_WORKING_C_

#include "tch.h"
#include "sdio_working.h"


tchStatus start_sdio_test(const tch_core_api_t* api)
{
	tch_sdioDevId devs[10];
	mset(devs, 0, sizeof(devs));

	tch_hal_module_sdio_t* sdio = api->Module->request(MODULE_TYPE_SDIO);

	tch_sdioCfg_t sdio_cfg;
	sdio->initCfg(&sdio_cfg, 4, ActOnSleep);
	tch_sdioHandle_t sdio_handle = sdio->alloc(api, &sdio_cfg, tchWaitForever);

	sdio_handle->deviceReset(sdio_handle);		// sdio send reset command

	uint32_t sdc_devcnt = sdio_handle->deviceId(sdio_handle, SDC, devs, 10);
	uint32_t mmc_devcnt = sdio_handle->deviceId(sdio_handle, MMC, devs, 10);

	api->Dbg->print(api->Dbg->Normal, 0 ,"%d SD Memory Card Found!!\n\r", sdc_devcnt);
	api->Dbg->print(api->Dbg->Normal, 0 ,"%d MMC Card Found!!\n\r",mmc_devcnt);

	return tchOK;
}



#endif /* SDIO_WORKING_C_ */
