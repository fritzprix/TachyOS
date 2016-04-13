/*
 * tch_interrupt.h
 *
 *  Created on: 2015. 8. 18.
 *      Author: innocentevil
 */

#ifndef TCH_INTERRUPT_H_
#define TCH_INTERRUPT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  interrupt priority has 5 level. so AAL should also support same scale
 *  you can give same priority for all level, if there is H/W limitation.
 *  otherwise, 5 distinct level is recommended
 *
 *  the level is lower is higher in priority
 *  - PRIORITY_0 is more urgent than PRIORITY_1
 *  - the highest is PRIORITY_0
 *  - the lowest is PRIORITY_4
 */
#define PRIORITY_0                     ((uint32_t) 0)
#define PRIORITY_1                     ((uint32_t) 1)
#define PRIORITY_2                     ((uint32_t) 2)
#define PRIORITY_3                     ((uint32_t) 3)
#define PRIORITY_4                     ((uint32_t) 4)

// optional handy macro for declaring isr handler
#define DECLARE_ISR_HANDLER(fname)      void fname(void)

typedef void (*isr_handler) (void);

extern void tch_remapTable(void* tblp);
extern tchStatus tch_enableInterrupt(IRQn_Type irq,uint32_t priority,isr_handler handler);
extern tchStatus tch_disableInterrupt(IRQn_Type irq);

#ifdef __cplusplus
}
#endif


#endif /* TCH_INTERRUPT_H_ */
