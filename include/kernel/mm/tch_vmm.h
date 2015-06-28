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

struct vm_dynamic_segement {

};

/**
 * vm_region is memory segment which is consist of contiguos pages. it is directly mapped to physical memory segment(mem_region)
 * when memory access faulted, kernel try to lookup vm_region tree for fault address. if fault address is mapped to current task
 * kernel performs various actions like load page into main memory (not present) / add page table entry (or permission table) depends on
 * cpu architecture.
 */
struct vm_region {
	// list or tree? for lookup
	void* 					vaddr;
	size_t					vsize;
	wt_alloc_t		 		alloc;
	struct mem_region*		pm_map;
};



void* tch_vallocRegion(struct vm_region* vreg,size_t sz);




#endif /* TCH_VMM_H_ */
