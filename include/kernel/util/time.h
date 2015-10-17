/*
 * time.h
 *
 *  Created on: 2015. 10. 17.
 *      Author: innocentevil
 */

#ifndef TIME_H_
#define TIME_H_

#include <stdint.h>

typedef uint64_t time_t;
struct tm {
	  int	tm_sec;
	  int	tm_min;
	  int	tm_hour;
	  int	tm_mday;
	  int	tm_mon;
	  int	tm_year;
	  int	tm_wday;
	  int	tm_yday;
	  int	tm_isdst;
};

extern time_t time(time_t* tloc);
extern struct tm* localtime(const time_t* timep,struct tm* result);
extern time_t mktime(struct tm* timep);

#endif /* TIME_H_ */
