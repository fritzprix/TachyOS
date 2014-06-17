/*
 * tch_port.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "port/acm4f/tch_port.h"
#include "core_cm4.h"

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


#define CTRL_PSTACK_FLAG           (uint32_t) (1 << 1)
#define CTRL_PSTACK_ENABLE         (uint32_t) (1 << 1)
#define CTRL_FPCA                  (uint32_t) (1 << 2)


#define IDLE_STACK_SIZE            (uint32_t) (1 << 9)
#define MAIN_STACK_SIZE            (uint32_t) (1 << 11)

#define SCB_AIRCR_KEY              (uint32_t) (0x5FA << SCB_AIRCR_VECTKEY_Pos)



static void tch_acm4_kernel_lock(void);
static void tch_acm4_kernel_unlock(void);
void tch_acm4_enableISR(void);
void tch_acm4_disableISR(void);
static void tch_acm4_switchContext(void* nth,void* cth) __attribute__((naked));
static void tch_acm4_saveContext(void* cth) __attribute__((naked));
static void tch_acm4_restoreContext(void* cth) __attribute__((naked));
static int tch_acm4_enterSvFromUsr(int sv_id,void* arg1,void* arg2);
static int tch_acm4_enterSvFromIsr(int sv_id,void* arg1,void* arg2);


/**
 *  	void (*_enableISR)(void);
	void (*_disableISR)(void);
	void (*_kernel_lock)(void);
	void (*_kernel_unlock)(void);

	void (*_switchContext)(void* nth,void* cth);
	void (*_saveContext)(void* cth);
	void (*_restoreContext)(void* cth);
	int (*_enterSvFromUsr)(int sv_id,void* arg1,void* arg2);
	int (*_enterSvFromIsr)(int sv_id,void* arg1,void* arg2);
 */
__attribute__((section(".data"))) static tch_port_ix _PORT_OBJ = {
		tch_acm4_kernel_lock,
		tch_acm4_kernel_unlock,
		tch_acm4_enableISR,
		tch_acm4_disableISR,
		tch_acm4_switchContext,
		tch_acm4_saveContext,
		tch_acm4_restoreContext,
		tch_acm4_enterSvFromUsr,
		tch_acm4_enterSvFromIsr
};




const tch_port_ix* tch_port_init(){

	SCB->AIRCR = (SCB_AIRCR_KEY | (6 << SCB_AIRCR_PRIGROUP_Pos));          /**  Set priority group
	                                                                        *   - [7] : Group Priority / [6:4] : Subpriority
	                                                                        *   - Handler or thread within same group priority
	                                                                        *     don't preempt each other
	                                                                        *   - Kenel thread has group priorty 1
	                                                                        *   - SuperVisor Mode has group priority 0
	                                                                        *   - highest priorty isr has group priority 1
	                                                                        *   -> Kernel thread isn't preempted by other isr
	                                                                        **/

	SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk);    /**
	                                                                                                       *  General Fault handler enable
	                                                                                                       *  - for debugging convinience
	                                                                                                       **/

	__set_PSP(__get_MSP());

	uint32_t mcu_ctrl = __get_CONTROL();
	mcu_ctrl |= CTRL_PSTACK_ENABLE;
#ifdef FEATURE_HFLOAT
	FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
	SCB->CPACR |= (0xF << 20);
#endif
	__set_CONTROL(mcu_ctrl);
	__ISB();

	return &_PORT_OBJ;
}




void tch_acm4_kernel_lock(void){
	__set_BASEPRI(MODE_KERNEL);
}


void tch_acm4_kernel_unlock(void){
	__set_BASEPRI(MODE_USER);
}

void tch_acm4_enableISR(void){
	__enable_irq();
}

void tch_acm4_disableISR(void){
	__disable_irq();
}

void tch_acm4_switchContext(void* nth,void* cth){

}

void tch_acm4_saveContext(void* cth){

}

void tch_acm4_restoreContext(void* cth){

}

int tch_acm4_enterSvFromUsr(int sv_id,void* arg1,void* arg2){

}

int tch_acm4_enterSvFromIsr(int sv_id,void* arg1,void* arg2){

}

