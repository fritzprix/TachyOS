/*
 * tch_syscfg.h
 *
 *  Created on: 2014. 11. 15.
 *      Author: innocentevil
 */

#ifndef TCH_SYSCFG_H_
#define TCH_SYSCFG_H_



#ifndef TCH_CFG_SHARED_MEM_SIZE
#define TCH_CFG_SHARED_MEM_SIZE            ((uint16_t) 0x2000)             // shared mem size : 8Kbyte
#endif

#ifndef TCH_CFG_THREAD_STACK_MIN_SIZE
#define TCH_CFG_THREAD_STACK_MIN_SIZE     ((uint16_t) 0x400)
#endif

#ifndef TCH_CFG_PROC_HEAP_SIZE
#define TCH_CFG_PROC_HEAP_SIZE            ((uint16_t) 0x1800)
#endif

#endif /* TCH_SYSCFG_H_ */
