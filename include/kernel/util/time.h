/*
 * time.h
 *
 *  Created on: 2015. 10. 17.
 *      Author: innocentevil
 */

#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>
#include "tch_types.h"


extern struct tm* tch_time_gmt_epoch_to_broken(const time_t* timep,struct tm* result,tch_timezone tz);
extern time_t tch_time_broken_to_gmt_epoch(struct tm* timep,tch_timezone tz);

#endif /* TIME_H_ */
