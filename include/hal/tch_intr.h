/*
 * tch_intr.h
 *
 *  Created on: 2014. 8. 15.
 *      Author: innocentevil
 */

#ifndef TCH_INTR_H_
#define TCH_INTR_H_

#include <stdint.h>

/**
 * #define HANDLER_SVC_PRIOR              (uint32_t)(GROUP_PRIOR(0) | SUB_PRIOR(0))
#define HANDLER_SYSTICK_PRIOR          (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(1))
#define HANDLER_HIGH_PRIOR             (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(2))
#define HANDLER_NORMAL_PRIOR           (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(3))
#define HANDLER_LOW_PRIOR              (uint32_t)(GROUP_PRIOR(1) | SUB_PRIOR(4))
 */

typedef struct tch_intr_prior {
	const uint32_t High;
	const uint32_t Normal;
	const uint32_t Low;
}tch_intr_priority;

typedef struct tch_lld_intr {
	tch_intr_priority Priority;
	void (*enable)(uint32_t irq);
	void (*disable)(uint32_t irq);
	void (*setPriority)(uint32_t irq,uint32_t priorty);
	uint32_t (*getPriority)(uint32_t irq);
	int (*isISR)();
}tch_lld_intr;

extern const tch_lld_intr* tch_interrupt_instance;


#endif /* TCH_INTR_H_ */
