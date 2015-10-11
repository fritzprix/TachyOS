/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"
#include "tch_i2c.h"


DECLARE_THREADROUTINE(main){

	tch_iicCfg iiccfg;
	iiccfg.Addr = 0xF2;
	iiccfg.AddrMode = IIC_ADDRMODE_7B;

	//tch_iicHandle* iic = env->Device->i2c->allocIIC();

	while(TRUE){
		ctx->uStdLib->stdio->iprintf("\r\nI'm Wakeup\n");
		ctx->Thread->sleep(1);
	}
}
