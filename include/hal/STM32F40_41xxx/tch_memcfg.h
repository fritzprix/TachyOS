/*
 * tch_memcfg.h
 *
 *  Created on: 2014. 7. 14.
 *      Author: innocentevil
 */

#ifndef TCH_MEMCFG_H_
#define TCH_MEMCFG_H_

#define __tch_Mem_Top 0x20020000
#define __tch_Mem_Bot 0x20000000

#if (!__tch_Mem_Top) || (!__tch_Mem_Bot)
 #error "__tch_Mem_Top mush be specified"
#endif


/**
 * @brief Main Stack Size : 8 KB
 */
#ifndef __tch_Main_Stack_Size
#define __tch_Main_Stack_Size        ((uint32_t) 1 << 13)
#endif


/**
 * @brief ISR Stack Size : 2KB
 *        Nesting in ISR is kinds of main cause of performance degeneration
 *        So 2KB is enough for short ISR routine
 */
#ifndef __tch_ISR_Stack_Size
#define __tch_ISR_Stack_Size          ((uint32_t) 1 << 11)
#endif

/**
 * @brief Heap Size : 62KB
 *
 */
#ifndef __tch_Heap_Size
#define __tch_Heap_Size               ((uint32_t) 1 << 16)
#endif

/**
 *
 */
#ifndef __tch_Idle_Stack_Size
#define __tch_Idle_Stack_Size         ((uint32_t) 1 << 10)
#endif

#endif /* TCH_MEMCFG_H_ */
