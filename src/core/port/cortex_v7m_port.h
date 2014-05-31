/*
 * stm_types.h
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */

#ifndef STM_TYPES_H_
#define STM_TYPES_H_

#include "tch_stdtypes.h"

#ifndef SYS_CLK
#define SYS_CLK 168000000
#endif



#define GROUP_PRIOR_Pos                (uint8_t) (7)
#define SUB_PRIOR_Pos                  (uint8_t) (4)
#define GROUP_PRIOR(x)                 (uint8_t) ((x & 1) << (GROUP_PRIOR_Pos - SUB_PRIOR_Pos))
#define SUB_PRIOR(y)                   (uint8_t) ((y & 7))

#define MODE_KERNEL                    (uint32_t)(1 << GROUP_PRIOR_Pos)                                 // execution priority of kernel only supervisor call can interrupt
#define MODE_USER                      (uint32_t)(0)

#define HANDLER_SVC_PRIOR              (uint32_t)(GROUP_PRIOR(0) | SUB_PRIOR(0))
#define HANDLER_SYSTICK_PRIOR          (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(1))
#define HANDLER_HIGH_PRIOR             (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(2))
#define HANDLER_NORMAL_PRIOR           (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(3))
#define HANDLER_LOW_PRIOR              (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(4))


#define THREAD_PRIORITY_KERNEL         (uint32_t) (5)
#define THREAD_PRIORITY_HIGH           (uint32_t) (4)
#define THREAD_PRIORITY_NORMAL         (uint32_t) (3)
#define THREAD_PRIORITY_LOW            (uint32_t) (2)
#define THREAD_PRIORITY_IDLE           (uint32_t) (1)

/**
 */

#define SVC_RETURNTO_THREAD            (int8_t) (0)
#define SVC_START_NEW_THREAD           (int8_t) (1)
#define SVC_TERMINATE_THREAD           (int8_t) (2)
#define SVC_PEND_WTO_THREAD            (int8_t) (3)
#define SVC_PEND_THREAD                (int8_t) (4)
#define SVC_WAKE_THREAD                (int8_t) (5)
#define SVC_JOIN_THREAD                (int8_t) (6)


#define SVC_MTX_LOCK                   (int8_t) (7)
#define SVC_MTX_UNLOCK                 (int8_t) (8)


/**
 *  ARMCM4 Dependent
 */
#define EPSR_THUMB_MODE                (uint32_t) (1 << 24)


#define _FMO_ISR_HANDLER(f_name) void name(void)
typedef struct _arm_sbrtn_cntx arm_sbrn_cntx;
typedef struct _arm_exc_stack arm_exc_stack;
typedef struct _fmo_exc_context fmo_exc_cntx;
typedef struct _fmo_thr_context fmo_thr_cntx;
/**
 * Follow ARM Procedure call standard
 *
 * contains local variable register should be preserved
 */
struct _arm_sbrtn_cntx{
	uint32_t R4;
	uint32_t R5;
	uint32_t R6;
	uint32_t R7;
	uint32_t R8;
	uint32_t R9;
	uint32_t R10;
	uint32_t R11;
	uint32_t LR;
};


/**
 * follows arm v7m arch. ref.
 * exception push & pop registers below in the stack at the entry and exit
 */
struct _arm_exc_stack{
	uint32_t R0;
	uint32_t R1;
	uint32_t R2;
	uint32_t R3;
	uint32_t R12;
	uint32_t LR14;
	uint32_t Return;
	uint32_t xPSR;
};

struct _fmo_exc_context{
	arm_exc_stack* R13;
};

struct _fmo_thr_context{
	arm_sbrn_cntx* R13;
};


int __sv_call(int8_t svc_type, uint32_t arg0,uint32_t arg1);
int __pndsv_call(int8_t svc_type,uint32_t arg0,uint32_t arg1);
void __pndsv_handled(void);

#endif /* STM_TYPES_H_ */
