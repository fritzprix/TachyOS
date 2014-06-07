/*
 * fmo_sys.h
 *
 *  Created on: 2014. 4. 13.
 *      Author: innocentevil
 */

#ifndef FMO_SYS_H_
#define FMO_SYS_H_

#include "fmo_thread.h"
#include "../util/generic_data_types.h"

#define TASK_PRIOR_EMERGENT     ((uint8_t) 4)
#define TASK_PRIOR_HIGH         ((uint8_t) 3)
#define TASK_PRIOR_NORMAL       ((uint8_t) 2)
#define TASK_PRIOR_LOW          ((uint8_t) 1)

typedef struct _tch_task_t tch_sysTask;
typedef struct _tch_sys tch_sys;
typedef void (*tch_systaskRoutin_t)(void*);
typedef struct _tch_sys_log tch_log;


#define DECLARE_TASK_FN(t_fn) void t_fn(void* arg)


struct _tch_sys_log{

};

struct _tch_task_t{
	tch_genericList_node_t  t_qnode;
	void*                  t_arg;
	uint8_t                t_prior;
	void (*t_routin)(void*);
};




struct _tch_sys {
	BOOL (*postSysTask)(tch_systaskRoutin_t t_routine, void* arg,uint8_t priority);
};



tchThread_t* sysThread;
BOOL tch_postSysTask(tch_systaskRoutin_t t_routine,void* arg, uint8_t priority);
void tch_print(const char* str);


#endif /* FMO_SYS_H_ */
