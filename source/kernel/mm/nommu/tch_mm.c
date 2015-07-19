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
#include "tch_loader.h"
#include "tch_kmalloc.h"

#ifndef CONFIG_NR_KERNEL_SEG
#define CONFIG_NR_KERNEL_SEG	5		// segment 0 dynamic /  segment 1 kernel text / segment 2 data / segment 3 sdata /  segment 4 kernel stack
#endif

struct tch_mm		init_mm;
/**
 *  kernel has initial mem_segment array which is declared as static variable,
 *  because there is not support of dynamic memory allocation in early kernel initialization stage.
 *
 */

void tch_mmInit(struct tch_mm* mmp){
	memset(mmp,0,sizeof(struct tch_mm));
	cdsl_dlistInit(&mmp->alc_list);
	mmp->mregions = NULL;
}

struct tch_mm* tch_mmProcInit(tch_thread_kheader* thread,struct tch_mm* mmp,struct proc_header* proc_header){
	/**
	 *  add mapping to region containing process binary image
	 */
	wt_heaproot_t* cache_root;
	wt_alloc_t* cache;
	tch_mmInit(mmp);
	/**
	 *  ================= setup regions for binary images ============================
	 *  1. dynamic program => dynamically loaded program in run time
	 *     - has binary image (text / data) in memory (RAM)
	 *     - has its own memory mapping node
	 *  2. static program  => statically linked with kernel binary image in build time
	 *     - binary image (text / data) is part of kernel
	 *     - has its own memory mapping node
	 *  3. child thread    => statically linekd with program binary of its root process
	 *     - binary image is part of program binary (whether kernel binary or user mode binary)
	 *     - share memory mapping with its root
	 */
	if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD) {
		mmp->mregions = NULL;
		if(proc_header->flag & PROCTYPE_DYNAMIC){			// dynamic loaded process
			mmp->text_region = proc_header->text_region;
			mmp->bss_region = proc_header->bss_region;
			mmp->data_region = proc_header->data_region;
		}else {
					// text/ bss / data section of static process are part of kernel binary image
					// and static process runs in privilidged mode (trusted process)
			mmp->text_region = NULL;
			mmp->bss_region = NULL;
			mmp->data_region = NULL;
		}
		mmp->mtx = Mtx->create();
		mmp->condv = Condv->create();
	}else {
		/**
		 * 	rb_treeNode_t*			mregions;			// region mapping node
		 * 	struct mem_region*		text_region;		// shared with parent
		 * 	struct mem_region*		bss_region;			// shared with parent
		 * 	struct mem_region*		data_region;		// shared with parent
		 * 	struct mem_region*		stk_region;			// has its own
		 * 	pgd_t*					pgd;				// has its own
		 * 	cdsl_dlistNode_t		alc_list;			// has its own
		 */

		struct tch_mm* parent_mm = tch_currentThread->t_kthread->t_parent->t_mm;
		memcpy(mmp,parent_mm,sizeof(struct tch_mm));
		mmp->pgd = tch_port_allocPageDirectory(kmalloc);
		if(mmp->text_region && mmp->bss_region && mmp->data_region){
			tch_port_addPageEntry(mmp->pgd, mmp->text_region->poff,mmp->text_region->flags);
			tch_port_addPageEntry(mmp->pgd, mmp->bss_region->poff,mmp->bss_region->flags);
			tch_port_addPageEntry(mmp->pgd, mmp->data_region->poff,mmp->data_region->flags);
		}
		cdsl_dlistInit(&mmp->alc_list);
	}

	/**
	 *  ================== Setup Stack region for process ==========================
	 *  1. allocate two regions struct which is memory managing sub unit
	 *    - heap / stack
	 *  2. stack region allocated from system dynamic segment
	 *  3. mapped to tch_mm struct for memory fault handling
	 */
	struct mem_region* regions = (struct mem_region*) kmalloc(sizeof(struct mem_region) * 2);
	struct mem_region* heap_region = &regions[0];
	mmp->stk_region = &regions[1];
	if(!proc_header->req_stksz)										// stack size should not 0
		proc_header->req_heapsz = TCH_CFG_THREAD_STACK_MIN_SIZE;
	if(!tch_segmentAllocRegion(0,mmp->stk_region,proc_header->req_stksz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_STACK)))
		goto MM_INIT_FAIL;
	tch_mapRegion(mmp,mmp->stk_region);
	tch_port_addPageEntry(mmp->pgd, mmp->stk_region->poff,mmp->stk_region->flags);


	/**
	 *  ============== prepare per thread user mode accessible struct ==============
	 *  1. located in stack bottom for check stack integrity
	 */
	tch_thread_uheader* uthread = (tch_thread_uheader*) (mmp->stk_region->poff << CONFIG_PAGE_SHIFT);
	thread->t_uthread = uthread;
	thread->t_uthread->t_kthread = thread;
	thread->t_uthread->t_fn = proc_header->entry;
	thread->t_uthread->t_destr = __tch_noop_destr;

	/***
	 *  ================= Prepare Process Argument in topper most area of stack =====================
	 *  Argument Type 1 : Null Terminated strings which is meant for command line argument from shell execution
	 *  Argument Type 2 : Pointer referencing to an object is meant for typical thread execution
	 */
	char* argv = (char*) (((size_t) mmp->stk_region->poff + mmp->stk_region->psz) >> CONFIG_PAGE_SHIFT);
	argv = argv - proc_header->argv_sz;
	if (proc_header->argv_sz > 0) {												// if process argument is null terminated strings,
		memcpy(argv, proc_header->argv, sizeof(char) * proc_header->argv_sz);   // copy them into stack top area
		thread->t_uthread->t_arg = argv;
	}else {																		// if process argument is just pointer to another object
		thread->t_uthread->t_arg = proc_header->argv;							// just copy refernece
	}
	mmp->estk = argv;
	if(proc_header->req_heapsz) {
		if(!tch_segmentAllocRegion(0,heap_region,proc_header->req_heapsz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_DYNAMIC)))
			goto MM_INIT_FAIL;
		tch_mapRegion(mmp,heap_region);
		cache_root = (wt_heaproot_t*) ((heap_region->poff + heap_region->psz) << CONFIG_PAGE_SHIFT);
		cache_root--;
		wtreeHeap_initCacheRoot(cache_root);
		cache = (wt_alloc_t*) cache_root;
		cache--;
		paddr_t sheap = (paddr_t) (heap_region->poff << CONFIG_PAGE_SHIFT);
		wtreeHeap_initCacheNode(cache,sheap,((size_t) cache -  (size_t) sheap));
		wtreeHeap_addCache(cache_root,cache);
		tch_port_addPageEntry(mmp->pgd,heap_region->poff,heap_region->flags);
		thread->t_uthread->t_cache = cache_root;
	}else {
		thread->t_uthread->t_cache = thread->t_parent->t_uthread->t_cache;
	}

	return mmp;

MM_INIT_FAIL:
	KERNEL_PANIC("tch_mm.c","mm struct init failed");
	return NULL;
}

int tch_mmProcClean(tch_thread_kheader* thread,struct tch_mm* mmp){

}



uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl){

	struct mem_segment* segment;
	int seg_id;
	struct section_descriptor** section = &mdesc_tbl[0];		// firts section descriptor should be normal
	if(((*section)->flags & SEGMENT_MSK) != SEGMENT_NORMAL)
		KERNEL_PANIC("tch_mm.c","invalid section descriptor table");

	tch_mmInit(&init_mm);
	tch_initSegment(*section);			// initialize segment manager and kernel dyanmic memory manager

	// register segments
	do {
		seg_id = tch_segmentRegister(*section);
		tch_mapSegment(&init_mm,seg_id);
		section++;
	} while (*section);

}


void tch_kernelOnMemFault(paddr_t pa, int fault){
	struct mem_region* region = tch_segmentGetRegionFromPtr(pa);
	if(perm_is_only_priv(region->flags) && !perm_is_public(region->flags) && region->owner != tch_currentThread->t_kthread->t_mm){
		//kill thread
		tch_schedThreadDestroy(tch_currentThread,tchErrorIllegalAccess);
	}

	if(!tch_port_addPageEntry(((struct tch_mm*) tch_currentThread->t_kthread->t_mm)->pgd, (region->poff << CONFIG_PAGE_SHIFT),get_permission(region->flags))) {			// add to table
		tch_schedThreadDestroy(tch_currentThread,tchErrorIllegalAccess);																// already in table?
	}
}

tchStatus __tch_noop_destr(tch_kobj* obj){return tchOK; }


