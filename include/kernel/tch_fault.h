/*
 * tch_fault.h
 *
 *  Created on: Jul 12, 2015
 *      Author: innocentevil
 */

#ifndef TCH_FAULT_H_
#define TCH_FAULT_H_

#define FAULT_MEM					((int)  0)
#define FAULT_BUS					((int)  1)
#define FAULT_ILLSTATE				((int)  3)
#define FAULT_HARD					((int)  4)


#define SPEC_DACCESS				((int)  0)
#define SPEC_IACCESS				((int)  1)
#define SPEC_STK					((int)  2)
#define SPEC_UNSTK					((int)  3)
#define SPEC_INVPC					((int)  4)
#define SPEC_UND					((int)  5)
#define SPEC_DIVZ					((int)  6)
#define SPEC_ALIGN					((int)  7)


#endif /* TCH_FAULT_H_ */
