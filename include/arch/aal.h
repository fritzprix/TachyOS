/*
 * aal.h
 *
 *  Created on: Apr 13, 2016
 *      Author: innocentevil
 */

#ifndef INCLUDE_ARCH_AAL_H_
#define INCLUDE_ARCH_AAL_H_


#include <stdint.h>
#include "tch_port.h"


typedef struct pte pte_t;
typedef pte_t* pmd_t;
typedef pmd_t* pgd_t;

typedef struct _tch_exc_stack tch_exc_stack;
typedef struct _tch_thread_context tch_thread_context;
typedef void* (*kernel_alloc_t) (size_t s);


#if (!defined(tch_port_setUserSP) || \
		!defined(tch_port_getUserSP) || \
		!defined(tch_port_setKernelSP) || \
		!defined(tch_port_getKernelSP))
#error "Stack Operation should be provided!!!"
#endif

extern BOOL tch_port_init();
extern void tch_port_setIsrVectorMap(uint32_t);


extern void tch_port_enableGlobalInterrupt(void);
extern void tch_port_disableGlobalInterrupt(void);
extern void tch_port_remapIsrTable(uwaddr_t remapped_table);
extern void tch_port_enableInterrupt(int32_t irq, uint32_t priority, void (*handler) (void));
extern void tch_port_disableInterrupt(int32_t irq);

/** \brief system lock from kernel to prevent kernel operation from being interrupted or preempted
 *  \note must be invoked in Kernel mode
 */
extern void tch_port_atomicBegin(void);

/** \brief system unlock from kernel to allow any interrupts or thread preemption
 *  \note must be invoked in kernel mode
 */
extern void tch_port_atomicEnd(void);

/** \brief thread has privilege access to hardware
 */
extern void tch_port_enablePrivilegedThread();

/** \brief thread doesn't have privilege access to hardware
 */
extern void tch_port_disablePrivilegedThread();

/** \brief check whether current execution context is in handler (or isr) mode
 */
extern BOOL tch_port_isISR();

/** \brief switch task (or thread) context
 *
 */
extern void tch_port_switch(void* nth,void* cth) __attribute__((naked,noreturn));



extern void tch_port_setJmp(uwaddr_t routine,uword_t arg1,uword_t arg2,uword_t arg3);
extern int tch_port_enterSv(word_t sv_id,uword_t arg1,uword_t arg2,uword_t arg3);
extern void* tch_port_makeInitialContext(uwaddr_t uthread_header,uwaddr_t sp,uwaddr_t initfn);
extern int tch_port_exclusiveCompareUpdate(uwaddr_t dest,uword_t comp,uword_t update);
extern int tch_port_exclusiveCompareDecrement(uwaddr_t dest,uword_t comp);
extern int tch_port_clearFault(int fault);
extern int tch_port_reset();

extern void tch_port_loadPageTable(pgd_t* pgd);
extern pgd_t* tch_port_allocPageDirectory(kernel_alloc_t alloc);
extern int tch_port_addPageEntry(pgd_t* pgd,uint32_t poffset,uint32_t flag);
extern int tch_port_removePageEntry(pgd_t* pgd,uint32_t poffset);


/**
 *
 */
extern int tch_port_setMemPermission(void* baddr,uint32_t sz,uint32_t permission);

/**
 *
 */
extern int tch_port_clrMemPermission(int id);


#endif /* INCLUDE_ARCH_AAL_H_ */
