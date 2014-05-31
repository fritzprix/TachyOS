/*
 * fmo_sched.h
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */

#ifndef FMO_SCHED_H_
#define FMO_SCHED_H_

#include "port/tch_stdtypes.h"
#include "port/cortex_v7m_port.h"
#include "fmo_thread.h"

typedef struct tch_sys tchSys;
typedef void (*sysTask_t)(void*);


/**
 * prevent all the interrupts,allowing svc call
 * so kernel guarantee critical section
 */
#define KERNEL_LOCK(){\
	__DMB();\
	__ISB();\
	__set_BASEPRI(MODE_KERNEL);\
}


/**
 * allow all the interrupts
 */
#define KERNEL_UNLOCK(){\
	__DMB();\
	__ISB();\
	__set_BASEPRI(MODE_USER);\
}



int sched_Init();

/**
 * public interface used in thread mode
 */
void sched_startThread(tchThread_t* thread);
void sched_wakeThreadFromQueue(tch_genericList_queue_t* src_queue);
void sched_pendCurrentThreadInWaitQueue(tch_genericList_queue_t* w_queue,tch_list_elementCompare policy);
void sched_pendCurrentThreadTimeout(uint32_t to);
void sched_pendCurrentJoinThread(tchThread_t* thread);
tchThread_t* sched_getCurrentThread(void);
uint64_t sched_currentTimeInMill(void);



#endif /* FMO_SCHED_H_ */
