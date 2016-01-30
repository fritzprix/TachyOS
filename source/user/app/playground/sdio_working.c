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
	tch_sdioHandle_t* sdio_handle = sdio->alloc(api, &sdio_cfg, tchWaitForever);
	sdio_handle->deviceId(sdio_handle, SDC, devs, 10);

	return tchOK;
}



#endif /* SDIO_WORKING_C_ */
