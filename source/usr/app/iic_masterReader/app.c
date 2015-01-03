/*
 * app.c
 *
 *  Created on: 2015. 1. 3.
 *      Author: innocentevil
 */


#include "tch.h"


DECLARE_THREADROUTINE(main){

	tch_iicCfg iiccfg;

	//tch_iicHandle* iic = env->Device->i2c->allocIIC();

	while(TRUE){
		env->uStdLib->stdio->iprintf("\r\nI'm Wakeup\n");
		env->Thread->sleep();
	}
}
