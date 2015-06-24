/*
 * tch_vmm.h
 *
 *  Created on: Jun 25, 2015
 *      Author: innocentevil
 */

#ifndef TCH_VMM_H_
#define TCH_VMM_H_

#include "tch_ktypes.h"
#include "wtmalloc.h"

struct vm_region {
	// list or tree? for lookup
	void* 			vaddr;
	size_t			vsize;
	wt_alloc_t 		alloc;
};



#endif /* TCH_VMM_H_ */
