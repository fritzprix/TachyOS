/*
 * tch_boot.h
 *
 *  Created on: 2015. 8. 4.
 *      Author: innocentevil
 */

#ifndef TCH_BOOT_H_
#define TCH_BOOT_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include "tch_mm.h"

#define BOOT_MAGIC				((int) 0xFF35FB)


typedef struct boot_param {
	int 								boot_magic;
	const char*						 	boot_name;
	int 								boot_id;
	int									boot_ver;				// upper 16 bit (major) / lower 16 bit (minor)
	struct section_descriptor** 		section_tbl;
}* bootp;

extern void tch_boot_setSystemClock();
extern void tch_boot_setSystemMem();


#if defined(__cplusplus)
}
#endif



#endif /* TCH_BOOT_H_ */
