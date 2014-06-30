/*
 * tch_thread.h
 *
 *  Created on: 2014. 6. 30.
 *      Author: innocentevil
 */

#ifndef TCH_THREAD_H_
#define TCH_THREAD_H_


/**
 *  Thread relevant type definition
 */
typedef void* tch_thread_id;
typedef struct _tch_thread_cfg_t tch_thread_cfg;
typedef void* (*tch_thread_routine)(void* arg);

typedef struct tch_msg{
	tch_thread_id thread;
	uint32_t      msg;
	void*        _arg;
} tch_msg;


typedef enum {
	Realtime = 5,
	High = 4,
	Normal = 3,
	Low = 2,
	Idle = 1
} tch_thread_prior;



struct _tch_thread_cfg_t {
	uint32_t             t_stackSize;
	tch_thread_routine  _t_routine;
	tch_thread_prior     t_proior;
	void*               _t_stack;
	const char*         _t_name;
};

/**
 * Thread Interface
 */


struct _tch_thread_ix_t {
	/**
	 *  Create Thread Object
	 */
	tch_thread_id (*create)(tch* sys,tch_thread_cfg* cfg,void* arg);
	/**
	 *  Start New Thread
	 */
	void (*start)(tch_thread_id thread);
	osStatus (*terminate)(tch_thread_id thread);
	tch_thread_id (*self)();
	osStatus (*sleep)(uint32_t millisec);
	osStatus (*join)(tch_thread_id thread);
	void (*setPriority)(tch_thread_prior nprior);
	tch_thread_prior (*getPriorty)();
};

#endif /* TCH_THREAD_H_ */
