/*
 * tch_kernel.h
 *
 *  Created on: 2014. 6. 22.
 *      Author: innocentevil
 */

#ifndef TCH_KERNEL_H_
#define TCH_KERNEL_H_

/***
 *   Kernel Interface
 *   - All kernel functions(like scheduling,synchronization) are based on this module
 *   - Initialize kernel enviroment (init kernel internal objects,
 */
#include "tch.h"
#include "lib/tch_absdata.h"
#include "port/acm4f/tch_port.h"
/***
 *  Supervisor call table
 */
typedef struct tch_prototype{
	tch tch_api;
	tch_port_ix*            tch_port;
} tch_prototype;


typedef enum tch_thread_state {
	PENDED = 1,                             // state in which thread is created but not started yet (waiting in ready queue)
	RUNNING = 2,                            // state in which thread occupies cpu
	WAIT = 3,                               // state in which thread wait for event (wake)
	SLEEP = 4,                              // state in which thread is yield cpu for given amount of time
	TERMINATED = -1                          // state in which thread has finished its task
} tch_thread_state;

typedef struct tch_thread_header {
	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_thread_routine          t_fn;
	const char*                 t_name;
	void*                       t_arg;
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	tch_thread_state            t_state;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint32_t                    t_id;
	uint64_t                    t_to;
	tch_thread_context*         t_ctx;
} tch_thread_header   __attribute__((aligned(4)));

typedef struct tch_thread_queue{
	tch_genericList_queue_t                  thque;
} tch_thread_queue;



#define SV_ROUTE_TO_KTHREAD               ((uint8_t) 1)

#define SV_THREAD_START                   (uint32_t) 0x20
#define SV_THREAD_TERMINATE               (uint32_t) 0x21
#define SV_THREAD_SLEEP                   (uint32_t) 0x22
#define SV_THREAD_JOIN                    (uint32_t) 0x23


extern void tch_kernelInit(void* arg);
extern void tch_kernelSysTick(void);
extern void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2);


#endif /* TCH_KERNEL_H_ */
