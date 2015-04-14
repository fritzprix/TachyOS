/*
 * tch_kcfg.h
 *
 *  Created on: 2014. 11. 15.
 *      Author: innocentevil
 */

#ifndef TCH_KCFG_H_
#define TCH_KCFG_H_


#ifndef TCH_ROUNDROBIN_TIMESLOT
#define TCH_ROUNDROBIN_TIMESLOT				((uint16_t) 10)
#endif

#ifndef TCH_CFG_SHARED_MEM_SIZE
#define TCH_CFG_SHARED_MEM_SIZE				((uint16_t) 0x2000)             // shared mem size : 8Kbyte
#endif

#ifndef TCH_CFG_KERNEL_HEAP_MEM_SIZE
#define TCH_CFG_KERNEL_HEAP_MEM_SIZE		((uint16_t) 0x2000)				// kernel mem size : 8Kbyte
#endif

#ifndef TCH_CFG_THREAD_STACK_MIN_SIZE
#define TCH_CFG_THREAD_STACK_MIN_SIZE		((uint16_t) 0x400)
#endif

#ifndef TCH_CFG_HEAP_MIN_SIZE
#define TCH_CFG_HEAP_MIN_SIZE				((uint16_t) 0x1800)
#endif

#ifndef TCH_KMEM_PAGE_SIZE
#define TCH_KMEM_PAGE_SIZE					((uint32_t) 0x400)  // default page size : 1024 byte
#endif

#endif /* TCH_KCFG_H_ */
