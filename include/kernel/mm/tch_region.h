/*
 * tch_region.h
 *
 *  Created on: Oct 15, 2016
 *      Author: innocentevil
 */

#ifndef INCLUDE_KERNEL_MM_TCH_REGION_H_
#define INCLUDE_KERNEL_MM_TCH_REGION_H_

#include "tch_segment.h"

#ifdef __cplusplus
extern "C" {
#endif

struct region_root {
	owtreeRoot_t            reg_pool;
	nrbtreeRoot_t          addr_rb;
};

struct region_node {
	nrbtreeNode_t          addr_rbn;
	uint32_t               flags;
	uint32_t               pstart;
	uint32_t               pend;
};


#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_KERNEL_MM_TCH_REGION_H_ */
