/*
 * time.c
 *
 *  Created on: 2015. 10. 17.
 *      Author: innocentevil
 */



#include "kernel/time.h"


#define SEC_PER_HOUR			((uint32_t) 3600)
#define SEC_PER_DAY				((uint32_t) SEC_PER_HOUR * 24)
#define SEC_PER_YEAR			((uint32_t) 31556926)
#define SEC_PER_WEEK			((uint32_t) SEC_PER_DAY * 7)

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

const uint8_t LEAP_YEAR_DAY_PER_MONTH[]  = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

const uint8_t NORMAL_YEAR_DAY_PER_MONTH[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};


/***
 *  gmt epoch time to broken time
 */
struct tm* tch_time_gmt_epoch_to_broken(const time_t* timep,struct tm* result,tch_timezone tz){
	if(!result || !timep)
		return NULL;
	time_t time = *timep;
	time += (tz * SEC_PER_HOUR);
	int year = 70;
	while(time > (_DAYS_IN_YEAR(year) * SEC_PER_DAY))
	{
		time -= (_DAYS_IN_YEAR(year) * SEC_PER_DAY);
		year++;
	}
	time = *timep + (tz * SEC_PER_HOUR);
	result->tm_year = year;
	result->tm_yday = (time % SEC_PER_YEAR) / SEC_PER_DAY;
	result->tm_hour = ((time % SEC_PER_DAY) /3600);
	result->tm_wday = (((time % SEC_PER_WEEK)) / SEC_PER_DAY - 3);
	result->tm_min = (time % 3600) / 60;
	result->tm_sec = time % 60;

	result->tm_mday = result->tm_yday + 1;
	result->tm_mon = 0;
	const uint8_t* month_vec = _ISLEAP(result->tm_year)? LEAP_YEAR_DAY_PER_MONTH : NORMAL_YEAR_DAY_PER_MONTH;
	while(result->tm_mday > month_vec[result->tm_mon]) {
		result->tm_mday -= month_vec[result->tm_mon++];
	}
	result->tm_isdst = -1;
	return result;
}


/**
 *  Local broken time to gmt epoch time (world time)
 */
time_t tch_time_broken_to_gmt_epoch(struct tm* timep,tch_timezone tz){
	if(!timep)
		return 0;

	time_t result = 0;
	int days = 0;
	int year;
	for(year = 70; year < timep->tm_year; year++){
		days += _DAYS_IN_YEAR(year);
	}
	result += (days * SEC_PER_DAY);
	result += timep->tm_yday * SEC_PER_DAY;
	result += timep->tm_hour * SEC_PER_HOUR;
	result += timep->tm_min * 60;
	result += timep->tm_sec;
	result -= (tz * SEC_PER_HOUR);
	return result;
}

