/*
 * tch_mm.c
 *
 *  Created on: 2015. 7. 5.
 *      Author: innocentevil
 */

#include "tch_mm.h"
#include "tch_err.h"
#include "tch_kernel.h"
#include "tch_segment.h"

#ifndef CONFIG_NR_KERNEL_SEG
#define CONFIG_NR_KERNEL_SEG	5		// segment 0 dynamic /  segment 1 kernel text / segment 2 data / segment 3 sdata /  segment 4 kernel stack
#endif

struct tch_mm		init_mm;
struct tch_mm*		current_mm;
/**
 *  kernel has initial mem_segment array which is declared as static variable,
 *  because there is not support of dynamic memory allocation in early kernel initialization stage.
 *
 */

struct tch_mm* tch_mmInit(struct tch_mm** mm,struct proc_header* proc_header){
	struct tch_mm* mmp = kmalloc(sizeof(struct tch_mm));
	*mm = mmp;
	cdsl_dlistInit(&mmp->alc_list);
	mmp->mregions = NULL;
	mmp->pgd = NULL;
	tch_mapRegion(mmp,proc_header->text_region);
	return mmp;
}


uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl){

	struct mem_segment* segment;
	int seg_id;
	struct section_descriptor** section = &mdesc_tbl[0];		// firts section descriptor should be normal
	if(((*section)->flags & SEGMENT_MSK) != SEGMENT_NORMAL)
		KERNEL_PANIC("tch_mm.c","invalid section descriptor table");

	tch_mmInit(&init_mm);
	current_mm = &init_mm;
	tch_initSegment(*section);			// initialize segment manager and kernel dyanmic memory manager

	// register segments
	do {
		seg_id = tch_segmentRegister(*section);
		tch_mapSegment(current_mm,seg_id);
		section++;
	} while (*section);

}


void tch_kernelOnMemFault(paddr_t pa, int fault){
	struct mem_region* region = tch_segmentGetRegionFromPtr(pa);
	if(perm_is_only_priv(region->flags)){
		//kill thread
		tch_schedThreadDestroy(tch_currentThread,tchErrorIllegalAccess);
	}
	if(perm_is_public(region->flags)){
		((struct tch_mm*) tch_currentThread->t_kthread->t_mm)->pgd = 1;
		if(!tch_port_addPageEntry(((struct tch_mm*) tch_currentThread->t_kthread->t_mm)->pgd, (region->poff << CONFIG_PAGE_SHIFT),get_permission(region->flags))) {			// add to table
			tch_schedThreadDestroy(tch_currentThread,tchErrorIllegalAccess);																// already in table?
		}
	}
	if(region->owner != tch_currentThread->t_kthread->t_mm){
		tch_schedThreadDestroy(tch_currentThread,tchErrorIllegalAccess);
	}
}

