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



#include "tch_TypeDef.h"
#include <time.h>

#if defined(__cplusplus)
extern "C"{
#endif

#define DECLARE_RTC_WKUP_FN(fn)           void fn(void)


typedef struct tch_rtc_handle tch_rtcHandle;
typedef uint32_t tch_alrId;
typedef enum {
	alrRes_DAY = ((uint8_t) 3),
	alrRes_HOUR = ((uint8_t) 2),
	alrRes_MIN = ((uint8_t) 1),
	alrRes_SEC = ((uint8_t) 0)
} tch_alrRes;
typedef void (*tch_rtc_wkupHandler)(void);


struct tch_rtc_handle {
	tchStatus (*close)(tch_rtcHandle* self);
	tchStatus (*setTime)(tch_rtcHandle* self,time_t gmt_epoch,tch_timezone tz,BOOL force);
	tchStatus (*getTime)(tch_rtcHandle* self,struct tm* ltm);
	tch_alrId (*setAlarm)(tch_rtcHandle* self,time_t alrtm,tch_alrRes resolution);
	tchStatus (*cancelAlarm)(tch_rtcHandle* self,tch_alrId alrm);
	tchStatus (*enablePeriodicWakeup)(tch_rtcHandle* self,uint16_t periodInSec,tch_rtc_wkupHandler handler);
	tchStatus (*disablePeriodicWakeup)(tch_rtcHandle* self);
};

typedef struct tch_lld_rtc {
	tch_rtcHandle* (*open)(const tch* env,time_t gmt_epoch,tch_timezone tz);
}tch_lld_rtc;

extern tch_lld_rtc* tch_rtcHalInit(const tch* env);

#if defined(__cplusplus)
}
#endif
#endif /* TCH_RTC_H_ */
