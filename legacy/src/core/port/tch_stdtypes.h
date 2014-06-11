/*
 * tch_stdtypes.h
 *
 *  Created on: 2014. 4. 19.
 *      Author: innocentevil
 */

#ifndef TCH_STDTYPES_H_
#define TCH_STDTYPES_H_

#include "stm32f4xx.h"

typedef enum {FALSE = 0,TRUE = 1} BOOL;
typedef enum _pwr_ctrl {ActOnSleep,DeactOnSleep} tch_pwrMgrCfg;
#define NULL               (void*)  0


#endif /* TCH_STDTYPES_H_ */
