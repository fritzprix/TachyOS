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


static void tch_port_enableSysTick(void);
static void tch_port_kernel_lock(void);
static void tch_port_kernel_unlock(void);
void tch_port_enableISR(void);
void tch_port_disableISR(void);
static void tch_port_switchContext(void* nth,void* cth) __attribute__((naked));
static void tch_port_saveContext(void* cth) __attribute__((naked));
static void tch_port_restoreContext(void* cth) __attribute__((naked));
static void tch_port_jmpToKernelModeThread(void* routine,uint32_t arg1,uint32_t arg2);
static int tch_port_enterSvFromUsr(int sv_id,uint32_t arg1,uint32_t arg2);
static int tch_port_enterSvFromIsr(int sv_id,uint32_t arg1,uint32_t arg2);
static void* tch_port_makeInitialContext(void* ctx,void* initfn);
static void tch_port_setThreadSP(uint32_t sp);
static uint32_t tch_port_getThreadSP(void);
static void tch_port_setHandlerSP(uint32_t sp);
static uint32_t tch_port_getHandlerSP(void);

static void __pend_loop(void) __attribute__((naked));


/**
 *
	void (*_enableISR)(void);
 */
__attribute__((section(".data"))) static tch_port_ix _PORT_OBJ = {
		tch_port_enableSysTick,
		tch_port_enableISR,
		tch_port_disableISR,
		tch_port_kernel_lock,
		tch_port_kernel_unlock,
		tch_port_switchContext,
		tch_port_saveContext,
		tch_port_restoreContext,
		tch_port_jmpToKernelModeThread,
		tch_port_enterSvFromUsr,
		tch_port_enterSvFromIsr,
		tch_port_makeInitialContext,
		tch_port_setThreadSP,
		tch_port_getThreadSP,
		tch_port_setHandlerSP,
		tch_port_getHandlerSP
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


void tch_port_enableSysTick(void){
	SysTick_Config(SYS_CLK / 1000);
}


void tch_port_kernel_lock(void){
	__set_BASEPRI(MODE_KERNEL);
}


void tch_port_kernel_unlock(void){
	__set_BASEPRI(MODE_USER);
}

void tch_port_enableISR(void){
	__enable_irq();
}

void tch_port_disableISR(void){
	__disable_irq();
}

void tch_port_switchContext(void* nth,void* cth){
	asm volatile(
			"push {r4-r11,lr}\n"
#ifdef FEATURE_HFLOAT
			"vpush {s16-s31}\n"
#endif
			"str sp,[%0]\n"

			"ldr sp,[%1]\n"
#ifdef FEATURE_HFLOAT
			"vpop {s16-s31}\n"
#endif
			"pop {r4-r11,lr}\n"
			"ldr r0,=%2\n"
			"svc #0" : : "r"(&((tch_thread_header*) cth)->t_ctx),"r"(&((tch_thread_header*) nth)->t_ctx),"i"(SV_EXIT_FROM_SV) :);
}

void tch_port_saveContext(void* cth){

}

void tch_port_restoreContext(void* cth){

}


/***
 *  this function redirect execution to thread mode for thread context manipulation
 */
void tch_port_jmpToKernelModeThread(void* routine,uint32_t arg1,uint32_t arg2){
	tch_exc_stack* org_sp = (tch_exc_stack*)__get_PSP();         /***
	                                                              *   prepare fake exception stack
	                                                              *    - passing arguement to kernel mode thread
	                                                              *    - redirect kernel routine
	                                                              **/
	org_sp--;                                                     // 1. push stack
	org_sp->R0 = arg1;                                            // 2. pass arguement into fake stack
	org_sp->R1 = arg2;
	org_sp->Return = (uint32_t)routine;                           // 3. modify return address of exc entry stack
	org_sp->xPSR = EPSR_THUMB_MODE;                               // 4. ensure returning to thumb mode
	__set_PSP((uint32_t)org_sp);                                            // 5. set manpulated exception stack as thread stack pointer
	tch_port_kernel_lock();                                       // 6. finally lock as kernel execution
}


int tch_port_enterSvFromUsr(int sv_id,uint32_t arg1,uint32_t arg2){
	int result = 0;
	asm volatile(
			"svc #0\n"                                // raise sv interrupt
			"str r0,[%0]" : : "r"(&result) :);        // return from sv interrupt and get result from register #0
	return result;
}

/***
 */
int tch_port_enterSvFromIsr(int sv_id,uint32_t arg1,uint32_t arg2){
	tch_exc_stack* org_sp = (tch_exc_stack*) __get_PSP();
	tch_memset(org_sp,sizeof(tch_exc_stack),0);
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
void* tch_port_makeInitialContext(void* th_header,void* initfn){
	tch_exc_stack* exc_sp = (tch_exc_stack*) th_header - 1;
	exc_sp->Return = (uint32_t)initfn;
	exc_sp->xPSR = EPSR_THUMB_MODE;
	exc_sp->R0 = 0;
	exc_sp->R1 = (uint32_t)th_header;
	tch_thread_context* th_ctx = (tch_thread_context*) exc_sp - 1;
	th_ctx->R4 = 0;
	th_ctx->R5 = 0;
	return th_ctx;
}

void tch_port_setThreadSP(uint32_t sp){
	__set_PSP(sp);
}

uint32_t tch_port_getThreadSP(void){
	return __get_PSP();
}

void tch_port_setHandlerSP(uint32_t sp){
	__set_MSP(sp);
}

uint32_t tch_port_getHandlerSP(void){
	return __get_MSP();
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
