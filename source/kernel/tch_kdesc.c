/*
 * tch_kdesc.c
 *
 *  Created on: 2015. 6. 7.
 *      Author: innocentevil
 */


#include "tch_kernel.h"
#include "tch_mm.h"

/*
 * 	int 			version_major;		// kernel version number
	int		 		version_minor;
	const char*		target_archid;
	const char*		target_plfid;
	size_t		 	target_memsz;
	size_t			k_stacksz;
	size_t			k_dynamicsz;
	uint32_t*		k_stacktop;
 */

#ifndef VER_MAJOR
#define VER_MAJOR	0
#endif

#ifndef VER_MINOR
#define VER_MINRO	0
#endif

#ifndef ARCH_NAME
#define ARCH_NAME "not specified"
#endif

#ifndef PLATFORM_NAME
#define PLATFORM_NAME "not specified"
#endif



tch_kernel_descriptor kernel_descriptor = {
		.version_major = VER_MAJOR,
		.version_minor = VER_MINRO,
		.arch_name = ARCH_NAME,
		.pltf_name = PLATFORM_NAME,
		.k_stacksz = CONFIG_KERNEL_STACKSIZE,
		.k_dynamicsz = CONFIG_KERNEL_DYNAMICSIZE,
		.k_stacktop = 0
};
