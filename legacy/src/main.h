/*
 * main.h
 *
 *  Created on: 2014. 2. 9.
 *      Author: innocentevil
 */

#ifndef MAIN_H_
#define MAIN_H_


#include "stm32f4xx.h"
#include "core_cm4.h"
#include "stm32f4xx_conf.h"
#include "hld/atcmd_bt.h"

void* main(void*);
void device_Init(void);

#endif /* MAIN_H_ */
