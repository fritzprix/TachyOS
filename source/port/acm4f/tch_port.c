/*
 * tch_port.c
 *
 *  Created on: 2014. 6. 13.
 *      Author: innocentevil
 */

#include "kernel/tch_kernel.h"
#include "port/acm4f/tch_port.h"
#include "lib/tch_lib.h"

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




#define IDLE_STACK_SIZE            (uint32_t) (1 << 9)
#define MAIN_STACK_SIZE            (uint32_t) (1 << 11)

#define SCB_AIRCR_KEY              (uint32_t) (0x5FA << SCB_AIRCR_VECTKEY_Pos)
#define EPSR_THUMB_MODE            (uint32_t) (1 << 24)

#define CTRL_PSTACK_ENABLE         (uint32_t) (1 << 1)
#define CTRL_FPCA                  (uint32_t) (1 << 2)



static void tch_acm4_kernel_lock(void);
static void tch_acm4_kernel_unlock(void);
void tch_acm4_enableISR(void);
void tch_acm4_disableISR(void);
static void tch_acm4_switchContext(uint32_t nxt_svid,void* nth,void* cth) __attribute__((naked));
static void tch_acm4_saveContext(uint32_t nxt_svid,void* cth) __attribute__((naked));
static void tch_acm4_restoreContext(uint32_t nxt_svid,void* cth) __attribute__((naked));
static void tch_acm4_returnToKernelModeThread(void* routine,uint32_t arg1,uint32_t arg2);
static int tch_acm4_enterSvFromUsr(int sv_id,uint32_t arg1,uint32_t arg2);
static int tch_acm4_enterSvFromIsr(int sv_id,uint32_t arg1,uint32_t arg2);
static void tch_acm4_makeInitialContext(void* ctx,void* initfn);


static void __pend_loop(void) __attribute__((naked));


/**
 */
__attribute__((section(".data"))) static tch_port_ix _PORT_OBJ = {
		tch_acm4_kernel_lock,
		tch_acm4_kernel_unlock,
		tch_acm4_enableISR,
		tch_acm4_disableISR,
		tch_acm4_switchContext,
		tch_acm4_saveContext,
		tch_acm4_restoreContext,
		tch_acm4_returnToKernelModeThread,
		tch_acm4_enterSvFromUsr,
		tch_acm4_enterSvFromIsr,
		tch_acm4_makeInitialContext
};




const tch_port_ix* tch_port_init(){
	__disable_irq();
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

	__set_PSP(__get_MSP());                         // Init Stack inherited to thread mode stack pointer
	uint32_t mcu_ctrl = __get_CONTROL();            /** Modify Control register
	                                                 *  - dedicated Thread Stack Pointer enabled
	                                                 *
	                                                 **/
	mcu_ctrl |= CTRL_PSTACK_ENABLE;
#ifdef FEATURE_HFLOAT
	/***
	 *   FPU Activation
	 */
	FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
	SCB->CPACR |= (0xF << 20);
#endif
	__set_CONTROL(mcu_ctrl);
	__ISB();


	// set Handler Priority
	NVIC_EnableIRQ(SysTick_IRQn);
	NVIC_EnableIRQ(SVCall_IRQn);
	NVIC_EnableIRQ(PendSV_IRQn);
	NVIC_SetPriority(SysTick_IRQn,HANDLER_SYSTICK_PRIOR);
	NVIC_SetPriority(SVCall_IRQn,HANDLER_SVC_PRIOR);
	NVIC_SetPriority(PendSV_IRQn,HANDLER_SVC_PRIOR);

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

void tch_acm4_switchContext(uint32_t svid,void* nth,void* cth){

}

void tch_acm4_saveContext(uint32_t svid,void* cth){

}

void tch_acm4_restoreContext(uint32_t svid,void* cth){
	asm volatile(
			"ldr sp,[%0]\n"
			"pop {r4-r11,lr}\n"
			"svc #0" : : "r"(((tch_thread_header*) cth)->t_ctx) :);
}


/***
 *  this function redirect execution to thread mode for thread context manipulation
 */
void tch_acm4_returnToKernelModeThread(void* routine,uint32_t arg1,uint32_t arg2){
	tch_exc_stack* org_sp = (tch_exc_stack*)__get_PSP();         /***
	                                                              *   prepare fake exception stack
	                                                              *    - passing arguement to kernel mode thread
	                                                              *    - redirect kernel routine
	                                                              **/
	org_sp--;                                                     // 1. push stack
	org_sp->R0 = SV_RETURN_TO_THREAD;                             // 2. pass arguement into fake stack
	org_sp->R1 = arg1;
	org_sp->R2 = arg2;
	org_sp->Return = (uint32_t)routine;                           // 3. modify return address of exc entry stack
	org_sp->xPSR = EPSR_THUMB_MODE;                               // 4. ensure returning to thumb mode
	__set_PSP((uint32_t)org_sp);                                            // 5. set manpulated exception stack as thread stack pointer
	tch_acm4_kernel_lock();                                       // 6. finally lock as kernel execution
}


int tch_acm4_enterSvFromUsr(int sv_id,uint32_t arg1,uint32_t arg2){
	int result = 0;
	asm volatile(
			"svc #0\n"                                // raise sv interrupt
			"str r0,[%0]" : : "r"(&result) :);        // return from sv interrupt and get result from register #0
	return result;
}

/***
 */
int tch_acm4_enterSvFromIsr(int sv_id,uint32_t arg1,uint32_t arg2){
	tch_exc_stack* org_sp = (tch_exc_stack*) __get_PSP();
	org_sp--;                                              // push stack to prepare manipulated stack for passing arguements to sv call(or handler)
	org_sp->R0 = sv_id;
	org_sp->R1 = arg1;
	org_sp->R2 = arg2;
	org_sp->Return = (uint32_t)__pend_loop;
	org_sp->xPSR = EPSR_THUMB_MODE;
	__set_PSP((uint32_t)org_sp);
	SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
	return 0;
}

/**
 *  prepare initial context for start thread
 */
void tch_acm4_makeInitialContext(void* sp,void* initfn){
	tch_thread_context* _sp = (tch_thread_context*) sp;
	tch_memset(sp,sizeof(tch_thread_context),0);
	_sp->LR = (uint32_t) initfn;
}


void __pend_loop(void){
	__ISB();
	__DMB();
	__WFI();
}

void SysTick_Handler(void){
	tch_kernelSysTick();
}


void SVC_Handler(void){
	tch_exc_stack* exsp = (tch_exc_stack*)__get_PSP();
	tch_kernelSvCall(exsp->R0,exsp->R1,exsp->R2);
}


void PendSV_Handler(void){
	tch_exc_stack* exsp = (tch_exc_stack*)__get_PSP();
	tch_exc_stack* org_sp = exsp + 1;
	__set_PSP((uint32_t)org_sp);                                       // pop manipulated stack
	tch_kernelSvCall(exsp->R0,exsp->R1,exsp->R2);
}
