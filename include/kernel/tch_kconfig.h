/*
 * tch_kconfig.h
 *
 *  Created on: 2015. 8. 5.
 *      Author: innocentevil
 */

#ifndef TCH_KCONFIG_H_
#define TCH_KCONFIG_H_




#ifndef CONFIG_KERNEL_STACKSIZE
#define CONFIG_KERNEL_STACKSIZE 	(4 << 10)
#endif

#ifndef CONFIG_HEAP_SIZE
#define CONFIG_HEAP_SIZE 			0x2000
#endif

#ifndef CONFIG_ROUNDROBIN_TIMESLOT
#define CONFIG_ROUNDROBIN_TIMESLOT  10
#endif

#ifndef CONFIG_MAX_CACHE_SIZE
#define CONFIG_MAX_CACHE_SIZE		0x800
#endif


#ifndef CONFIG_KERNEL_DYNAMICSIZE
#define CONFIG_KERNEL_DYNAMICSIZE 	(16 << 10)
#endif

#ifndef CONFIG_PAGE_SHIFT
#define CONFIG_PAGE_SHIFT			12
#endif

#ifndef CONFIG_THREAD_MIN_STACK
#define CONFIG_THREAD_MIN_STACK		0x400
#endif

#ifndef CONFIG_SHM_SIZE
#define CONFIG_SHM_SIZE				(1 << 14)
#endif


#endif /* TCH_KCONFIG_H_ */
