/*
 * tch_interrupt.h
 *
 *  Created on: 2015. 8. 18.
 *      Author: innocentevil
 */

#ifndef TCH_INTERRUPT_H_
#define TCH_INTERRUPT_H_


extern void tch_remapTable(void* tblp);
extern tchStatus tch_enableInterrupt(IRQn_Type irq,uint32_t priority);
extern tchStatus tch_disableInterrupt(IRQn_Type irq);


#endif /* TCH_INTERRUPT_H_ */
