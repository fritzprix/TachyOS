/*
 * fmo_sched.c
 *
 *  Created on: 2014. 3. 21.
 *      Author: innocentevil
 */


#include "fmo_sched.h"
#include "mmap.h"
#include "../util/tch_lib.h"
#include "../util/generic_data_types.h"
#include "fmo_synch.h"
#include "fmo_time.h"
#include "fmo_task.h"
#include "port/cortex_v7m_port.h"

/**
 * Macro Variable
 */






#define CTRL_PSTACK_FLAG           (uint32_t) (1 << 1)
#define CTRL_PSTACK_ENABLE         (uint32_t) (1 << 1)
#define CTRL_FPCA                  (uint32_t) (1 << 2)


#define IDLE_STACK_SIZE            (uint32_t) (1 << 9)
#define MAIN_STACK_SIZE            (uint32_t) (1 << 11)

#define SCB_AIRCR_KEY              (uint32_t) (0x5FA << SCB_AIRCR_VECTKEY_Pos)


#define INIT_QUEUE(q) {\
	q.entry = NULL;\
	q.tail = NULL;\
}

#define AS_LIST_NODE(th_p)             ((tch_genericList_node_t*) th_p)
#define AS_THREAD(g_node_p)            ((tchThread_t*) g_node_p)
#define PUT_THREAD_TO_QUEUE(q_p,th_p)  (tch_genericQue_enqueueWithCompare(q_p,AS_LIST_NODE(th_p),thread_priority_policy))
#define NEXT_THREAD(th_p)              AS_THREAD(AS_LIST_NODE(th_p)->next)
#define PREV_THREAD(th_p)              AS_THREAD(AS_LIST_NODE(th_p)->prev)


extern void* main(void*);


//private variable
static tchThread_t* cthread;                                                    /// ptr of current thread
/*
static tchThread_queue th_rq;                                                   /// ready queue
static tchThread_queue th_pq;                                                   /// pend queue*/
static tch_genericList_queue_t th_rq;                                           /// ready queue of thread
static tch_genericList_queue_t th_pq;
static volatile uint64_t sys_timeTick;                                                  /// system tick cnt

static uint32_t idleStack[IDLE_STACK_SIZE];                                    /// stack required by idle thread
static uint32_t mainStack[MAIN_STACK_SIZE];
static tchThread_t* idleThread;                                                 /// Idle thread
static tchThread_t* mainThread;                                                 /// main thread which has main function as a routine


//private function
static void sv_startCurrentThread(void);                                        /// *start current thread
static void sched_terminateCThread(void);                                       /// *invoked when thread finish task
static void sv_returnToKernelRoutine(void* routine);                            /// *return to thread mode with directing given function
static int sv_isPreemtable(void);                                               /// *check current thread is to be preempted
static void sv_reschedule(void);                                                /// *put current thread into ready queue & dequeue next thread to run and make it current thread


static void sched_switchContext(void) __attribute__((naked));                   /// *switch context of two thread which both already have started
static void sched_saveAndStart(void) __attribute__((naked));                    /// *
static void sched_resumeContext(void) __attribute__((naked));

#ifdef THR_TRACE_ENABLE
static void recordHandlerEntry();
static void recordHandlerReturn();
#endif

static THREAD_ROUTINE(idle);                                                     /// Idle Thread Routine

static LIST_COMPARE_FN(thread_priority_policy);
static LIST_COMPARE_FN(thread_tprioritize_policy);

static int preemptCnt;




int sched_Init(){
	KERNEL_LOCK();
	cthread = NULL;
	preemptCnt = 0;
	/**
	 * Test Purpose
	 */
	SCB->CCR |= SCB_CCR_NONBASETHRDENA_Msk;
	SCB->AIRCR = (SCB_AIRCR_KEY | (6 << SCB_AIRCR_PRIGROUP_Pos));
	SCB->SHCSR |= (SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk | SCB_SHCSR_USGFAULTENA_Msk);



	INIT_QUEUE(th_rq);
	INIT_QUEUE(th_pq);


	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
	SysTick_Config(SYS_CLK / 1000);


	tchThread_t * __init_sp = (tchThread_t*)( __mthread_stack_top - 1);
	//stack initialize
	__set_PSP((uint32_t)__init_sp);\

	//put thread mode
	uint32_t mcu_ctrl = __get_CONTROL();
	mcu_ctrl |= CTRL_PSTACK_ENABLE;
#ifdef FEATURE_HFLOAT
	FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk;
	SCB->CPACR |= (0xF << 20);
#endif
	__set_CONTROL(mcu_ctrl);
	__ISB();

	//now thread mode

	sys_timeTick = 0;

	idleThread = tchThread_create(idleStack,IDLE_STACK_SIZE,idle,THREAD_PRIORITY_IDLE,NULL,"Idle");
	mainThread = tchThread_create((uint32_t*)mainStack,MAIN_STACK_SIZE,(void* (*)(void*))main,THREAD_PRIORITY_NORMAL,NULL,"main");


	SET_THREAD_STATUS(mainThread,THREAD_STATUS_READY);
	SET_THREAD_STATUS(idleThread,THREAD_STATUS_READY);
	SET_THREAD_STATUS(sysThread,THREAD_STATUS_READY);

	PUT_THREAD_TO_QUEUE(&th_rq,idleThread);
	PUT_THREAD_TO_QUEUE(&th_rq,mainThread);
	PUT_THREAD_TO_QUEUE(&th_rq,sysThread);


	NVIC_EnableIRQ(SysTick_IRQn);
	NVIC_EnableIRQ(SVCall_IRQn);
	NVIC_EnableIRQ(PendSV_IRQn);
	NVIC_SetPriority(SysTick_IRQn,HANDLER_SYSTICK_PRIOR);
	NVIC_SetPriority(SVCall_IRQn,HANDLER_SVC_PRIOR);
	NVIC_SetPriority(PendSV_IRQn,HANDLER_SVC_PRIOR);
	__enable_irq();
	cthread = AS_THREAD(tch_genericQue_dequeue(&th_rq));
	sv_startCurrentThread();
	while(1){
		__WFI();
	}
	/// unreachable code
	return SUCCESS;
}


/**
 * ================ scheduler public function ==================
 */

void sched_startThread(tchThread_t* thread){
	if(!__get_IPSR()){
		__sv_call(SVC_START_NEW_THREAD,(uint32_t)thread,0);
	}else{
		__pndsv_call(SVC_START_NEW_THREAD,(uint32_t)thread,0);
	}
}

void sched_pendCurrentThreadInWaitQueue(tch_genericList_queue_t* w_queue,tch_list_elementCompare queue_policy){
	if(queue_policy == NULL) queue_policy = thread_priority_policy;
	if(!__get_IPSR()){
		__sv_call(SVC_PEND_THREAD,(uint32_t) w_queue,(uint32_t)queue_policy);
	}else{
		__pndsv_call(SVC_PEND_THREAD,(uint32_t) w_queue,(uint32_t) queue_policy);
	}
}

void sched_pendCurrentThreadTimeout(uint32_t to){
	if(!__get_IPSR()){
		__sv_call(SVC_PEND_WTO_THREAD,to,0);
	}else{
		__pndsv_call(SVC_PEND_WTO_THREAD,to,0);
	}
}
void sched_wakeThreadFromQueue(tch_genericList_queue_t* src_queue){
	if(!__get_IPSR()){
		__sv_call(SVC_WAKE_THREAD,(uint32_t)src_queue,0);
	}else{
		__pndsv_call(SVC_WAKE_THREAD,(uint32_t)src_queue,0);
	}
}

void sched_pendCurrentJoinThread(tchThread_t* thread){
	if(!__get_IPSR()){
		__sv_call(SVC_JOIN_THREAD,(uint32_t) thread,0);
	}else{
		__pndsv_call(SVC_JOIN_THREAD,(uint32_t) thread,0);
	}
}

uint64_t sched_currentTimeInMill(void){
	return sys_timeTick;
}

tchThread_t* sched_getCurrentThread(void){
	return cthread;
}



void sv_returnToKernelRoutine(void* routine){
	arm_exc_stack* stack = (arm_exc_stack*) __get_PSP();
	stack--;
	stack->R0 = 0;
	stack->Return = (uint32_t) routine;
	stack->xPSR = EPSR_THUMB_MODE;
	__set_PSP((uint32_t) stack);
	__DMB();
	__ISB();
	return;
}

void sv_startCurrentThread(void){
	/**
	 * Change Thread state -> Started/Active : means it is run and has context to save
	 */
	SET_THREAD_STARTED(cthread);
	SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);

	__set_PSP((uint32_t)cthread->t_tctx.R13);            ///            load stack pointer of current thread
	__DMB();
	__ISB();
#ifdef FEATURE_HFLOAT
	asm volatile("vldr s0,=%0" : : "i"(0) :);            ///           intentionally create floating point context
	                                                     /***
	                                                      *            this floating point instruction guarantees that every context of thread
	                                                      *            is floating point mode. it means that identical exception stack
	                                                      *            can be assumed. it's not best way to optimize context switching performance
	                                                      *            of kernel though, it's quite simple in structure of the code.
	                                                      *
	                                                      *            Cortex M4F series support lazy save feature for floating point
	                                                      *            context switching performance. so overhead from making all the thread
	                                                      *            have floating point mode is not too significant	                                                      *
	                                                      */
#endif
	KERNEL_UNLOCK();                                     ///            unlock privililedged operation return to noraml(user) mode
	cthread->t_fn(cthread->t_arg);                                 ///            start thread routine
	sched_terminateCThread();                            ///            invoked when thread is done
}


/**
 * put current thread into ready queue and dequeu
 */
void sv_reschedule(){
	tchThread_t* oth = cthread;
	SET_THREAD_STATUS(oth,THREAD_STATUS_READY);
	PUT_THREAD_TO_QUEUE(&th_rq,oth);
	cthread = AS_THREAD(tch_genericQue_dequeue(&th_rq));
	AS_LIST_NODE(cthread)->prev = AS_LIST_NODE(oth);
	SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);
}


/**
 *
 * check current thread should be preempted based on the priority
 */
int sv_isPreemtable(){
	//if requested status change
	return (cthread->t_prior <= AS_THREAD(th_rq.entry)->t_prior);
}


/**
 * context
 */
void __attribute__((naked)) sched_switchContext(void){
	asm volatile(
			"push {r4-r11,lr}\n"
#ifdef FEATURE_HFLOAT
			"vpush {s0-s15}\n"
#endif
			"str sp,[%1]\n"

			"ldr sp,[%0]\n"
#ifdef FEATURE_HFLOAT
			"vpop {s0-s15}\n"
#endif
			"pop  {r4-r11,lr}\n"
			"ldr r0,=%2\n"
			"dmb\n"
			"isb\n"
			"svc #0" : :"r"(&cthread->t_tctx.R13),"r"(&PREV_THREAD(cthread)->t_tctx.R13),"i"(SVC_RETURNTO_THREAD) :
	);
}


void __attribute__((naked)) sched_saveAndStart(void){
	asm volatile(
			"push {r4-r11,lr}\n"
#ifdef FEATURE_HFLOAT
			"vpush {s0-s15}\n"
#endif
			"str sp,[%0]\n"
			"dmb\n"
			"isb\n" : :"r"(&PREV_THREAD(cthread)->t_tctx.R13) :
	);
	sv_startCurrentThread();
}

void __attribute__((naked)) sched_resumeContext(void){
	asm volatile(
			"ldr sp,[%0]\n"
#ifdef FEATURE_HFLOAT
			"vpop {s0-s15}\n"
#endif
			"pop {r4-r11,lr}\n"
			"ldr r0,=%1\n"
			"dmb\n"
			"isb\n"
			"svc #0" : : "r"(&cthread->t_tctx.R13),"i"(SVC_RETURNTO_THREAD) :
	);
}


void sched_terminateCThread(void){
	__sv_call(SVC_TERMINATE_THREAD,(uint32_t)cthread,0);
}

void PendSV_Handler(void){
#ifdef THR_TRACE_ENABLE
	recordHandlerEntry();
#endif
	arm_exc_stack* stack = (arm_exc_stack*) __get_PSP();
	uint32_t arg0 = stack->R0;
	uint32_t arg1 = stack->R1;
	uint32_t arg2 = stack->R2;
	stack++;
	__set_PSP((uint32_t) stack);
	tchThread_t* nth = NULL;
	switch(arg0){
	case SVC_RETURNTO_THREAD:                                                       /// Discard Kernel Operation Stack and return to original thread : No Arg
		KERNEL_UNLOCK();
		break;
	case SVC_START_NEW_THREAD:                                                      /// Start New Thread, new thread can preempt current thread based on their priority, otherwise put in ready queue : Arg0 (New Thread)
		if(cthread->t_prior < ((tchThread_t*) arg1)->t_prior){
			AS_LIST_NODE(arg1)->prev = AS_LIST_NODE(cthread);
			cthread = ((tchThread_t*)arg1);
			SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,PREV_THREAD(cthread));
			sv_returnToKernelRoutine(sched_saveAndStart);
			KERNEL_LOCK();
		}else{
			SET_THREAD_STATUS(((tchThread_t*) arg1),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,AS_THREAD(arg1));
		}
		break;
	case SVC_TERMINATE_THREAD:                                                      /// Terminate Current Thread and Resume next thread context (wake thread) : No Arg
		CLEAR_THREAD_STARTED(cthread);
		SET_THREAD_STATUS(cthread,THREAD_STATUS_TERMINATED);
		while(cthread->t_joinQ.entry != NULL){
			nth = AS_THREAD(tch_genericQue_dequeue(&cthread->t_joinQ));
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
		}
		cthread = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		if(IS_THREAD_STARTED(cthread)){
			SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);
			sv_returnToKernelRoutine(sched_resumeContext);
		}else{
			sv_returnToKernelRoutine(sv_startCurrentThread);
		}
		KERNEL_LOCK();
		break;
	case SVC_PEND_THREAD:
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*) arg1,AS_LIST_NODE(cthread),(tch_list_elementCompare) arg2);
		nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
		cthread = nth;
		SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_PEND);
		if(IS_THREAD_STARTED(cthread)){
			sv_returnToKernelRoutine(sched_switchContext);
		}else{
			sv_returnToKernelRoutine(sched_saveAndStart);
		}
		KERNEL_LOCK();
		break;
	case SVC_PEND_WTO_THREAD:                                                       /// Put current thread in pending (or wait) state with given timeout value : Arg1 ( Timeout Value)
		nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
		cthread = nth;
		SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_PEND);
		PREV_THREAD(cthread)->t_to = sys_timeTick + (uint64_t) arg1;
		tch_genericQue_enqueueWithCompare(&th_pq,AS_LIST_NODE(PREV_THREAD(cthread)),thread_tprioritize_policy);
		if(IS_THREAD_STARTED(cthread)){
			sv_returnToKernelRoutine(sched_switchContext);
		}else{
			sv_returnToKernelRoutine(sched_saveAndStart);
		}
		KERNEL_LOCK();
		break;
	case SVC_WAKE_THREAD:                                                            /// Wake thread from given wait queue and schedule it : Arg1 (Thread Queue)
		nth = AS_THREAD(tch_genericQue_dequeue(((tch_genericList_queue_t*) arg1)));
		if(cthread->t_prior < nth->t_prior){
			((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
			cthread = nth;
			SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,PREV_THREAD(cthread));
			if(IS_THREAD_STARTED(cthread)){
				sv_returnToKernelRoutine(sched_switchContext);
			}else{
				sv_returnToKernelRoutine(sched_saveAndStart);
			}
			KERNEL_LOCK();
		}else{
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
			KERNEL_UNLOCK();
		}
		break;
	case SVC_JOIN_THREAD:                                                            /// if target thread has not terminated yet, current thread will be put on the Wait Queue until target is terminated, otherwise just return
		nth = (tchThread_t*) arg1;
		if(GET_THREAD_STATUS(nth) == THREAD_STATUS_TERMINATED){
			return;
		}else{
			SET_THREAD_STATUS(cthread,THREAD_STATUS_PEND);
			PUT_THREAD_TO_QUEUE(&nth->t_joinQ,cthread);
			nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
			((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
			cthread = nth;
			if(IS_THREAD_STARTED(cthread)){
				SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);
				sv_returnToKernelRoutine(sched_switchContext);
			}else{
				sv_returnToKernelRoutine(sched_saveAndStart);
			}
			KERNEL_LOCK();
		}
		break;
	case SVC_MTX_LOCK:                                                               /// try lock mutex : Arg1 (Mutex)
		if(arg2){
			if(!((tch_mtx_lock*)arg1)->key){
				((tch_mtx_lock*) arg1)->key = (uint32_t) cthread;
				if(!cthread->t_lckCnt++){
					cthread->t_svd_prior = cthread->t_prior;                             /// to prevent thread being preemted critical section
					cthread->t_prior = THREAD_PRIORITY_KERNEL;
				}
				break;
			}else{
				nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
				((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
				cthread = nth;
				PUT_THREAD_TO_QUEUE((tch_genericList_queue_t*)&((tch_mtx_lock*) arg1)->w_q,PREV_THREAD(cthread));
				if(IS_THREAD_STARTED(nth)){
					sv_returnToKernelRoutine(sched_switchContext);
				}else{
					sv_returnToKernelRoutine(sched_saveAndStart);
				}
				KERNEL_LOCK();
				break;
			}
		}else{
			//no wait mtx lock
			if(!((tch_mtx_lock*)arg1)->key){
				((tch_mtx_lock*) arg1)->key = (uint32_t) cthread;
				if(!cthread->t_lckCnt++){
					cthread->t_svd_prior = cthread->t_prior;                             /// to prevent thread being preemted critical section
					cthread->t_prior = THREAD_PRIORITY_KERNEL;
				}
				stack->R0 = TRUE;
			}else{
				stack->R0 = FALSE;
			}
		}
		break;
	case SVC_MTX_UNLOCK:                                                          /// unlock mutex and give the key to most prior thread in the wait queue of this mutex
		if(!(--cthread->t_lckCnt)){
			cthread->t_prior = cthread->t_svd_prior;
		}
		((tch_mtx_lock*) arg1)->key = 0;
		if(((tch_mtx_lock*)arg1)->w_q.entry != NULL){
			nth = (tchThread_t*) tch_genericQue_dequeue((tch_genericList_queue_t*)&((tch_mtx_lock*)arg1)->w_q);
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
		}
		KERNEL_UNLOCK();
		break;
	}
#ifdef THR_TRACE_ENABLE
	recordHandlerReturn();
#endif
}

void SVC_Handler(void){                                                                  /// Exactly same to PendSV except for stack operation
#ifdef THR_TRACE_ENABLE
	recordHandlerEntry();
#endif
	arm_exc_stack* stack = (arm_exc_stack*) __get_PSP();
	tchThread_t* nth = NULL;
	switch(stack->R0){
	case SVC_RETURNTO_THREAD:                                                       /// Discard Kernel Operation Stack and return to original thread : No Arg
		stack++;
		__set_PSP((uint32_t) stack);
		KERNEL_UNLOCK();
		break;
	case SVC_START_NEW_THREAD:                                                      /// Start New Thread, new thread can preempt current thread based on their priority, otherwise put in ready queue : Arg0 (New Thread)
		if(cthread->t_prior < ((tchThread_t*) stack->R1)->t_prior){
			AS_LIST_NODE(stack->R1)->prev = AS_LIST_NODE(cthread);
			cthread = ((tchThread_t*)stack->R1);
			SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,PREV_THREAD(cthread));
			sv_returnToKernelRoutine(sched_saveAndStart);
			KERNEL_LOCK();
		}else{
			SET_THREAD_STATUS(((tchThread_t*) stack->R1),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,AS_THREAD(stack->R1));
		}
		break;
	case SVC_TERMINATE_THREAD:                                                      /// Terminate Current Thread and Resume next thread context (wake thread) : No Arg
		CLEAR_THREAD_STARTED(cthread);
		SET_THREAD_STATUS(cthread,THREAD_STATUS_TERMINATED);
		while(cthread->t_joinQ.entry != NULL){
			nth = AS_THREAD(tch_genericQue_dequeue(&cthread->t_joinQ));
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
		}
		cthread = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		if(IS_THREAD_STARTED(cthread)){
			SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);
			sv_returnToKernelRoutine(sched_resumeContext);
		}else{
			sv_returnToKernelRoutine(sv_startCurrentThread);
		}
		KERNEL_LOCK();
		break;
	case SVC_PEND_WTO_THREAD:                                                       /// Put current thread in pending (or wait) state with given timeout value : Arg1 ( Timeout Value)
		nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
		cthread = nth;
		SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_PEND);
		PREV_THREAD(cthread)->t_to = sys_timeTick + (uint64_t) stack->R1;
		tch_genericQue_enqueueWithCompare(&th_pq,AS_LIST_NODE(PREV_THREAD(cthread)),thread_tprioritize_policy);
		if(IS_THREAD_STARTED(cthread)){
			sv_returnToKernelRoutine(sched_switchContext);
		}else{
			sv_returnToKernelRoutine(sched_saveAndStart);
		}
		KERNEL_LOCK();
		break;
	case SVC_PEND_THREAD:
		tch_genericQue_enqueueWithCompare((tch_genericList_queue_t*) stack->R1,AS_LIST_NODE(cthread),(tch_list_elementCompare) stack->R2);
		nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
		((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
		cthread = nth;
		SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_PEND);
		if(IS_THREAD_STARTED(cthread)){
			sv_returnToKernelRoutine(sched_switchContext);
		}else{
			sv_returnToKernelRoutine(sched_saveAndStart);
		}
		KERNEL_LOCK();
		break;
	case SVC_WAKE_THREAD:                                                            /// Wake thread from given wait queue and schedule it : Arg1 (Thread Queue)
		nth = AS_THREAD(tch_genericQue_dequeue(((tch_genericList_queue_t*) stack->R1)));
		if(cthread->t_prior < nth->t_prior){
			((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
			cthread = nth;
			SET_THREAD_STATUS(PREV_THREAD(cthread),THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,PREV_THREAD(cthread));
			if(IS_THREAD_STARTED(cthread)){
				sv_returnToKernelRoutine(sched_switchContext);
			}else{
				sv_returnToKernelRoutine(sched_saveAndStart);
			}
			KERNEL_LOCK();
		}else{
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
		}
		break;
	case SVC_JOIN_THREAD:                                                            /// if target thread has not terminated yet, current thread will be put on the Wait Queue until target is terminated, otherwise just return
		nth = (tchThread_t*) stack->R1;
		if(GET_THREAD_STATUS(nth) == THREAD_STATUS_TERMINATED){
			break;
		}else{
			SET_THREAD_STATUS(cthread,THREAD_STATUS_PEND);
			PUT_THREAD_TO_QUEUE(&nth->t_joinQ,cthread);
			nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
			((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
			cthread = nth;
			if(IS_THREAD_STARTED(cthread)){
				SET_THREAD_STATUS(cthread,THREAD_STATUS_ACTIVE);
				sv_returnToKernelRoutine(sched_switchContext);
			}else{
				sv_returnToKernelRoutine(sched_saveAndStart);
			}
			KERNEL_LOCK();
		}
		break;
	case SVC_MTX_LOCK:                                                               /// try lock mutex : Arg1 (Mutex)
		if(stack->R2){
			if(!((tch_mtx_lock*)stack->R1)->key){
				((tch_mtx_lock*) stack->R1)->key = (uint32_t) cthread;
				if(!cthread->t_lckCnt++){
					cthread->t_svd_prior = cthread->t_prior;                             /// save original priority of thread
					cthread->t_prior = THREAD_PRIORITY_KERNEL;
				}
				break;
			}else{
				nth = AS_THREAD(tch_genericQue_dequeue(&th_rq));
				((tch_genericList_node_t*)nth)->prev = AS_LIST_NODE(cthread);
				cthread = nth;
				PUT_THREAD_TO_QUEUE((tch_genericList_queue_t*)&((tch_mtx_lock*) stack->R1)->w_q,PREV_THREAD(cthread));
				if(IS_THREAD_STARTED(nth)){
					sv_returnToKernelRoutine(sched_switchContext);
				}else{
					sv_returnToKernelRoutine(sched_saveAndStart);
				}
				KERNEL_LOCK();
				break;
			}
		}else{
			// no wait mode mtx lock
			if(!((tch_mtx_lock*) stack->R1)->key){
				((tch_mtx_lock*)stack->R1)->key = (uint32_t) cthread;
				if(!cthread->t_lckCnt++){
					cthread->t_svd_prior = cthread->t_prior;
					cthread->t_prior = THREAD_PRIORITY_KERNEL;
				}
				stack->R0 = TRUE;
			}else{
				stack->R0 = FALSE;
			}
		}
		break;
	case SVC_MTX_UNLOCK:                                                         /// unlock mutex and give the key to most prior thread in the wait queue of this mutex
		((tch_mtx_lock*) stack->R1)->key = 0;
		if(!(--cthread->t_lckCnt)){
			cthread->t_prior = cthread->t_svd_prior;
		}
		if(((tch_mtx_lock*)stack->R1)->w_q.entry != NULL){
			nth = (tchThread_t*) tch_genericQue_dequeue((tch_genericList_queue_t*)&((tch_mtx_lock*)stack->R1)->w_q);
			SET_THREAD_STATUS(nth,THREAD_STATUS_READY);
			PUT_THREAD_TO_QUEUE(&th_rq,nth);
		}
		break;
	}
#ifdef THR_TRACE_ENABLE
	recordHandlerReturn();
#endif
}

void Systick_Vector(void){
#ifdef THR_TRACE_ENABLE
	recordHandlerEntry();
#endif
	tchThread_t* nth = NULL;
	sys_timeTick++;
	if(!cthread->t_tslot--)cthread->t_tslot = THREAD_TIME_SLOT;
	if(sv_isPreemtable()){
		cthread->t_tslot = THREAD_TIME_SLOT;
		sv_reschedule();
		if(IS_THREAD_STARTED(cthread)){
			sv_returnToKernelRoutine(sched_switchContext);
		}else{
			sv_returnToKernelRoutine(sched_saveAndStart);
		}
		KERNEL_LOCK();
	}
	while((th_pq.entry != NULL) && (AS_THREAD(th_pq.entry)->t_to < sys_timeTick)){
		nth = AS_THREAD(tch_genericQue_dequeue(&th_pq));
		PUT_THREAD_TO_QUEUE(&th_rq,nth);
		SET_THREAD_STATUS(nth,THREAD_STATUS_READY);

	}
#ifdef THR_TRACE_ENABLE
	recordHandlerReturn();
#endif
}

#ifdef THR_TRACE_ENABLE
void recordHandlerEntry(){
	uint8_t ctr_idx = cthread->t_traceIdx;
	struct _exc_boundary_info_t* exc_info = &cthread->t_trace[ctr_idx].entry_info;
	exc_info->excid = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
	exc_info->nonbase = ((SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) == 0);
	exc_info->exc_stackp = (arm_exc_stack*) __get_PSP();
	fmo_memcpy((uint8_t*)&exc_info->exc_stack,(uint8_t*) exc_info->exc_stackp,sizeof(arm_exc_stack));
	fmo_memcpy((uint8_t*)&exc_info->exc_outter_stack,(uint8_t*)((arm_exc_stack*)exc_info->exc_stackp + 1),sizeof(arm_exc_stack));
	exc_info->ts = sched_currentTimeInMill();
	exc_info->cthread = sched_getCurrentThread();
}

void recordHandlerReturn(){
	uint8_t ctr_idx = cthread->t_traceIdx;
	struct _exc_boundary_info_t* exc_info = &cthread->t_trace[ctr_idx].exit_info;
	exc_info->excid = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);
	exc_info->nonbase = ((SCB->ICSR & SCB_ICSR_RETTOBASE_Msk) == 0);
	exc_info->exc_stackp = (arm_exc_stack*) __get_PSP();
	fmo_memcpy((uint8_t*)&exc_info->exc_stack,(uint8_t*) exc_info->exc_stackp,sizeof(arm_exc_stack));
	fmo_memcpy((uint8_t*)&exc_info->exc_outter_stack,(uint8_t*)((arm_exc_stack*)exc_info->exc_stackp + 1),sizeof(arm_exc_stack));
	exc_info->ts = sched_currentTimeInMill();
	exc_info->cthread = sched_getCurrentThread();
	cthread->t_traceIdx++;
	if(cthread->t_traceIdx == THR_TRACE_SIZE){
		cthread->t_traceIdx = 0;
	}
}
#endif

LIST_COMPARE_FN(thread_priority_policy){
	return (((tchThread_t* )li0)->t_prior < ((tchThread_t*) li1)->t_prior);
}
LIST_COMPARE_FN(thread_tprioritize_policy){
	return AS_THREAD(li0)->t_to > AS_THREAD(li1)->t_to;
}

THREAD_ROUTINE(idle){

	while(1){
		//on sleep
		__DMB();
		__ISB();
		__WFI();
		__DMB();
		__ISB();
		//on wakeup
	}
	return NULL;
}



