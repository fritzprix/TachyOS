/*
 * tch_systimer.h
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 7. 1.
 *      Author: innocentevil
 */

#ifndef TCH_TIME_H_
#define TCH_TIME_H_

#include "tch_TypeDef.h"

#if defined(__cplusplus)
extern "C"{
#endif

extern tch_systime_ix* tch_systimeInit(uint64_t timeInMills);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_SYSTIMER_H_ */
