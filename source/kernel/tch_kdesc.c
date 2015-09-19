/*
 * tch_kdesc.c
 *
 *  Copyright (C) 2014 doowoong,lee
 *  All rights reserved.
 *
 *  This software may be modified and distributed under the terms
 *  of the LGPL v3 license.  See the LICENSE file for details.
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
