/*
 * fmo_time.h
 *
 *  Created on: 2014. 4. 6.
 *      Author: innocentevil
 */

#ifndef FMO_TIME_H_
#define FMO_TIME_H_

#include "port/cortex_v7m_port.h"

typedef struct _system_time_t vtimer_t;
typedef void (*timeEventListener)(uint32_t r_id);
struct _system_time_t {
	int (*start)(timeEventListener listener);
	int (*setTimeout)(uint32_t r_id,uint16_t timeIn_ms);
	int (*stop)(void);
};

const vtimer_t* vtimer;

#endif /* FMO_TIME_H_ */
