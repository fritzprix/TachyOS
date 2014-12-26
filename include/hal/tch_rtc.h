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

typedef enum {
	UTC_N12 = ((int8_t) -12),
	UTC_N11 = ((int8_t) -11),
	UTC_N10 = ((int8_t) -10),
	UTC_N9 =  ((int8_t) -9),
	UTC_N8 =  ((int8_t) -8),
	UTC_N7 =  ((int8_t) -7),
	UTC_N6 =  ((int8_t) -6),
	UTC_N5 =  ((int8_t) -5),
	UTC_N4 =  ((int8_t) -4),
	UTC_N3 =  ((int8_t) -3),
	UTC_N2 =  ((int8_t) -2),
	UTC_N1 =  ((int8_t) -1),
	UTC_0 =   ((int8_t) 0),
	UTC_P1 =  ((int8_t) 1),
	UTC_P2 =  ((int8_t) 2),
	UTC_P3 =  ((int8_t) 3),
	UTC_P4 =  ((int8_t) 4),
	UTC_P5 =  ((int8_t) 5),
	UTC_P6 =  ((int8_t) 6),
	UTC_P7 =  ((int8_t) 7),
	UTC_P8 =  ((int8_t) 8),
	UTC_P9 =  ((int8_t) 9),
	UTC_P10 = ((int8_t) 10),
	UTC_P11 = ((int8_t) 11),
	UTC_P12 = ((int8_t) 12),
	UTC_P13 = ((int8_t) 13),
	UTC_P14 = ((int8_t) 14)
}tch_timezone;

typedef struct tch_rtc_handle tch_rtcHandle;
typedef uint32_t tch_alrId;




struct tch_rtc_handle {
	tchStatus (*close)(tch_rtcHandle* self);
	tchStatus (*setTime)(tch_rtcHandle* self,time_t localtime);
	time_t (*getTime)(tch_rtcHandle* self);
	tch_alrId (*setAlarm)(tch_rtcHandle* self,time_t alrtm);
	tchStatus (*cancelAlarm)(tch_rtcHandle* self,tch_alrId alrm);
	tchStatus (*enablePeriodicWakeup)(tch_rtcHandle* self,uint32_t periodInSec);
	tchStatus (*disablePeriodicWakeup)(tch_rtcHandle* self);
	tchStatus (*waitUntilWakeup)(tch_rtcHandle* self);
};

typedef struct tch_lld_rtc {
	tch_rtcHandle* (*open)(const tch* env,time_t lt,tch_timezone tz);
}tch_lld_rtc;

extern const tch_lld_rtc* tch_rtc_instance;

#endif /* TCH_RTC_H_ */
