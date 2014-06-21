/*
 * tch_kernel.h
 *
 *  Created on: 2014. 6. 22.
 *      Author: innocentevil
 */

#ifndef TCH_KERNEL_H_
#define TCH_KERNEL_H_

#include "tch.h"
#include "lib/tch_absdata.h"
#include "port/acm4f/tch_port.h"
/***
 *  Supervisor call table
 */
#define SV_RETURN_TO_THREAD                 ((uint8_t) 1)

#define SV_THREAD_START                   (uint32_t) 0x20
#define SV_THREAD_TERMINATE               (uint32_t) 0x21
#define SV_THREAD_SLEEP                   (uint32_t) 0x22
#define SV_THREAD_JOIN                    (uint32_t) 0x23

typedef struct tch_thread_header {
	tch_genericList_node_t      t_listNode;
	tch_genericList_queue_t     t_joinQ;
	tch_thread_routine          t_fn;
	const char*                 t_name;
	void*                       t_arg;
	uint32_t                    t_lckCnt;
	uint32_t                    t_tslot;
	uint32_t                    t_status;
	uint32_t                    t_prior;
	uint32_t                    t_svd_prior;
	uint32_t                    t_id;
	uint64_t                    t_to;
	tch_thread_context*         t_ctx;
} tch_thread_header   __attribute__((aligned(4)));

extern void tch_kernelInit(void* arg);
extern BOOL tch_kernelStart(void* arg);
extern void tch_kernelSysTick(void);
extern void tch_kernelSvCall(uint32_t sv_id,uint32_t arg1, uint32_t arg2);


#endif /* TCH_KERNEL_H_ */
