/*
 * tch_rtc.h
 *
 * Copyright (C) 2014 doowoong,lee
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the LGPL v3 license.  See the LICENSE file for details.
 *
 *
 *  Created on: 2014. 6. 15.
 *      Author: innocentevil
 */

#ifndef TCH_RTC_H_
#define TCH_RTC_H_


#include "tch_ktypes.h"
#include "tch_types.h"


#if defined(__cplusplus)
extern "C"{
#endif

#define DECLARE_RTC_WKUP_FN(fn)           void fn(void)


typedef struct tch_rtc_handle tch_rtcHandle;

typedef void (*tch_rtc_wkupHandler)(void);
struct tch_rtc_handle {
	tchStatus (*close)(tch_rtcHandle* self);
	tchStatus (*setTime)(tch_rtcHandle* self,struct tm* localtm,BOOL force);
	tchStatus (*getTime)(tch_rtcHandle* self,struct tm* localtm);
	tchStatus (*enablePeriodicWakeup)(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler handler);
	tchStatus (*disablePeriodicWakeup)(tch_rtcHandle* self);
};

typedef struct tch_hal_module_rtc {
	tch_rtcHandle* (*open)(const tch_core_api_t* env,struct tm* localtm);
}tch_hal_module_rtc_t;

#if defined(__cplusplus)
}
#endif
#endif /* TCH_RTC_H_ */
