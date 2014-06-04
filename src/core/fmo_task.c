/*
 * fmo_sys.c
 *
 *  Created on: 2014. 4. 13.
 *      Author: innocentevil
 */



#include "fmo_task.h"
#include "fmo_thread.h"
#include "fmo_synch.h"
#include "fmo_sched.h"
#include "port/tch_stdtypes.h"





#ifndef TASK_BUFFER_LENGTH
#define TASK_BUFFER_LENGTH                 (uint16_t) (1 << 6)
#endif



/***
 *  type definition
 */
typedef uint32_t sys_stack[1 << 10];

/**
 *
 */
struct sys_thread {
	sys_stack  thread_stack;
	tchThread_t thread;
};



static void* system_threadRoutine(void* arg);
static LIST_COMPARE_FN(task_prioritize_policy);
static BOOL tch_task_isAvailable(tch_sysTask* task);
static void*   tch_task_freeTask   (tch_sysTask* task);
static DECLARE_TASK_FN(tch_task_bootStrap);

static __attribute__((section(".data"))) tch_sysTask SYS_INIT_TASK = {
		{NULL,NULL},
		NULL,
		THREAD_PRIORITY_KERNEL,
		tch_task_bootStrap
};
static __attribute__((section(".data")))tch_genericList_queue_t sys_taskQ = {
		&SYS_INIT_TASK,
		&SYS_INIT_TASK
};

static __attribute__((section(".data")))tch_genericList_queue_t sys_threadHolder = {
		NULL,
		NULL
};


static const char* sysThreadName = "sysThread";

/**
 * 	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_thread_routine_t        t_fn;
	const char*                 t_name;
	void*                       t_arg;
	uint8_t                     t_lckCnt;
	uint32_t                    t_tslot;
	uint32_t                    t_status;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint32_t                    t_id;
	uint64_t                    t_to;
	fmo_thr_cntx                t_tctx;
 */
static __attribute__((section(".data"))) struct sys_thread SYSTHREAD = {
		{0,},                                                /// Thread Stack Area
		{                                                    /// System Thread Structure
				GENERIC_LIST_NODE_INIT,
				GENERIC_LIST_QUEUE_INIT,
				system_threadRoutine,
				"sysThread",
				NULL,
				0,
				THREAD_TIME_SLOT,
				THREAD_STATUS_DEACTIVE,
				THREAD_PRIORITY_KERNEL,
				THREAD_PRIORITY_KERNEL,
				(uint32_t)&SYSTHREAD.thread_stack,
				0,
				{(arm_sbrn_cntx*)&SYSTHREAD.thread}
		}
};

static tch_sysTask TASK_BUFFER[TASK_BUFFER_LENGTH];

static __attribute__((section(".data"))) tch_generic_ringBuffer TASK_MEM_POOL = {
		TASK_BUFFER,
		sizeof(tch_sysTask),
		TASK_BUFFER_LENGTH,
		0,
		{
				(BOOL (*)(void*)) tch_task_isAvailable,
				(void* (*)(void*)) tch_task_freeTask
		}

};

tchThread_t* sysThread = &SYSTHREAD.thread;




BOOL tch_postSysTask(tch_systaskRoutin_t t_routine,void* arg, uint8_t priority){
	tch_sysTask* nTask = tch_genericMemPool_allcate(&TASK_MEM_POOL);
	if(nTask == NULL){
		return FALSE;
	}
	nTask->t_arg = arg;
	nTask->t_prior = priority;
	nTask->t_routin = t_routine;
	tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*)&sys_taskQ,(tch_genericList_node_t*) nTask,task_prioritize_policy);
	if(sys_threadHolder.entry){
		sched_wakeThreadFromQueue(&sys_threadHolder);
	}
	return TRUE;
}

void tch_task_bootStrap(void* arg){
	tch_genericQue_Init((tch_genericList_queue_t*)&sys_taskQ);
	tch_genericQue_Init((tch_genericList_queue_t*)&sys_threadHolder);
	tch_genericMemPool_init(&TASK_MEM_POOL,TASK_BUFFER,sizeof(tch_sysTask),TASK_BUFFER_LENGTH);
}


BOOL tch_task_isAvailable(tch_sysTask* task){
	return task->t_routin == NULL;
}

void*   tch_task_freeTask   (tch_sysTask* task){
	task->t_routin = NULL;
	task->t_prior = 0;
	task->t_qnode.next = NULL;
	task->t_qnode.prev = NULL;
	task->t_arg = NULL;
	return (void*) task;
}

void* system_threadRoutine(void* arg){
	tch_sysTask* task = NULL;
	void* rVal_p = NULL;
	while(1){
		while(sys_taskQ.entry != NULL){
			task = (tch_sysTask*) tch_genericQue_dequeue(&sys_taskQ);
			task->t_routin(task->t_arg);
			tch_genericMemPool_free(&TASK_MEM_POOL,task);
		}
		sched_pendCurrentThreadInWaitQueue(&sys_threadHolder,NULL);
	}
	return NULL;
}


LIST_COMPARE_FN(task_prioritize_policy){
	return ((tch_sysTask*)li0)->t_prior > ((tch_sysTask*)li1)->t_prior;
}
