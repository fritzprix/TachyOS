/*
 * tch_kdesc.c
 *
 *  Created on: 2015. 6. 7.
 *      Author: innocentevil
 */


#include "kernel/tch_kernel.h"


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



const tch_kernel_descriptor kernel_descriptor = {
		.version_major = VER_MAJOR,
		.version_minor = VER_MINRO,
		.arch_name = ARCH_NAME,
		.pltf_name = PLATFORM_NAME,
};
