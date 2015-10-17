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


#if defined(__cplusplus)
extern "C"{
#endif

extern tch_time_ix Time_IX;


extern void tch_systimeInit(const tch* env,time_t itm,tch_timezone itz);
extern tch_timezone tch_systimeSetTimeZone(tch_timezone itz);
extern tchStatus tch_systimeSetTimeout(tch_threadId thread,uint32_t timeout,tch_timeunit tu);
extern tchStatus tch_systimeCancelTimeout(tch_threadId thread);
extern BOOL tch_systimeIsPendingEmpty();

#if defined(__cplusplus)
}
#endif

#endif /* TCH_SYSTIMER_H_ */
