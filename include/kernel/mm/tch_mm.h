/*
 * tch_mm.h
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */

#ifndef TCH_MM_H_
#define TCH_MM_H_

#include "tch_ktypes.h"



#ifdef __GNUC__
#define __kstack	__attribute__((section(".kernel.stack")))
#elif __IAR__
#endif


#ifdef __GNUC__
#define __kdynamic	__attribute__((section(".kernel.dynamic")))
#elif __IAR__
#endif







#endif /* TCH_MM_H_ */
