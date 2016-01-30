/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"
#include "tch_i2c.h"


DECLARE_THREADROUTINE(main){

	tch_hal_module_iic_t i2c = ctx->Module->request(MODULE_TYPE_IIC);

	tch_iicCfg iiccfg;
	iiccfg.Addr = 0xF2;
	iiccfg.AddrMode = IIC_ADDRMODE_7B;

	while(TRUE){
		ctx->Thread->sleep(1);
	}
}
