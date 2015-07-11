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

struct tch_mm* tch_mmInit(struct tch_mm* mmp){
	cdsl_dlistInit(&mmp->alc_list);
	mmp->mregions = NULL;
	mmp->pgd = NULL;
	return mmp;
}


uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl){

	struct mem_segment* segment;
	int seg_id;
	struct section_descriptor** section = &mdesc_tbl[0];		// firts section descriptor should be normal
	if(((*section)->flags & SECTION_MSK) != SECTION_NORMAL)
		KERNEL_PANIC("tch_mm.c","invalid section descriptor table");

	tch_mmInit(&init_mm);
	current_mm = &init_mm;
	tch_initSegment(*section);			// initialize segment manager and kernel dyanmic memory manager

	// register rest segment
	do {
		seg_id = tch_segmentRegister(*section);
		tch_mapSegment(current_mm,seg_id);
		section++;
	} while (!section);

}


void tch_kernelOnMemFault(){

}

