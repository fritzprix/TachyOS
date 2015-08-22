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
#include "tch_mtx.h"
#include "tch_condv.h"
#include "kernel/tch_kconfig.h"

#ifndef CONFIG_NR_KERNEL_SEG
#define CONFIG_NR_KERNEL_SEG	5		// segment 0 dynamic /  segment 1 kernel text / segment 2 data / segment 3 sdata /  segment 4 kernel stack
#endif

#define SECTION_KERNEL_DYNAMIC

struct user_heap {
	wt_heapRoot_t		heap_root;
	wt_heapNode_t		heap_node;
};



volatile struct tch_mm*		current_mm;
struct tch_mm				init_mm;


static uint32_t* init_mmProcStack(struct tch_mm* mmp,struct mem_region* stkregion,size_t stksz);

BOOL tch_mmProcInit(tch_thread_kheader* thread,struct proc_header* proc_header){
	/**
	 *  add mapping to region containing process binary image
	 */
	wt_heapRoot_t* heap_root;
	wt_heapNode_t* init_heap;
	tch_mtxCb* mtx;
	tch_condvCb* condv;
	wt_cache_t* cache;
	struct tch_mm* mmp = &thread->mm;
	memset(mmp,0,sizeof(struct tch_mm));
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
		mmp->pgd = tch_port_allocPageDirectory(kmalloc);
		mmp->dynamic = (struct proc_dynamic*) kmalloc(sizeof(struct proc_dynamic));
		mtx = (tch_mtxCb*) kmalloc(sizeof(tch_mtxCb));
		condv = (tch_condvCb*) kmalloc(sizeof(tch_condvCb));
		if(!mmp->dynamic || !mmp->pgd || !mtx || !condv){
			kfree(condv);
			kfree(mtx);
			kfree(mmp->pgd);
			kfree(mmp->dynamic);
			return FALSE;
		}

		if(proc_header->flag & PROCTYPE_DYNAMIC){			// dynamic loaded process
			mmp->text_region = proc_header->text_region;
			mmp->bss_region = proc_header->bss_region;
			mmp->data_region = proc_header->data_region;
		}else {
					// text/ bss / data section of static process are assumed to be part of kernel binary image
			mmp->text_region = NULL;
			mmp->bss_region = NULL;
			mmp->data_region = NULL;
		}

		mmp->dynamic->mregions = NULL;
		mmp->dynamic->mtx = tch_mutexInit(mtx,FALSE);
		mmp->dynamic->condv = tch_condvInit(condv,FALSE);
	}else {
		struct tch_mm* parent_mm = &tch_currentThread->kthread->parent->mm;
		memcpy(mmp,parent_mm,sizeof(struct tch_mm));
		mmp->pgd = tch_port_allocPageDirectory(kmalloc);
		if(!mmp->pgd)
			return FALSE;
		mmp->dynamic = parent_mm->dynamic;
		if(mmp->text_region && mmp->bss_region && mmp->data_region){
			tch_port_addPageEntry(mmp->pgd, mmp->text_region->poff,mmp->text_region->flags);
			tch_port_addPageEntry(mmp->pgd, mmp->bss_region->poff,mmp->bss_region->flags);
			tch_port_addPageEntry(mmp->pgd, mmp->data_region->poff,mmp->data_region->flags);
		}
		mmp->kobjs = NULL;
	}

	/**
	 *  ================== Setup Stack region for process ==========================
	 *  1. allocate two regions struct which is memory managing sub unit
	 *    - heap / stack
	 *  2. stack region allocated from system dynamic segment
	 *  3. mapped to tch_mm struct for memory fault handling
	 */
	struct mem_region* regions = (struct mem_region*) kmalloc(sizeof(struct mem_region) * 2);
	mmp->heap_region = &regions[0];
	mmp->stk_region = &regions[1];

	if(!init_mmProcStack(mmp,mmp->stk_region,proc_header->req_stksz)){
		if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD){
			kfree(condv);
			kfree(mtx);
		}
		kfree(regions);
		kfree(mmp->pgd);
		kfree(mmp->dynamic);
		return FALSE;
	}

	/**
	 *  ================== Stack layout =========================
	 *  |  thread argment										|
	 *  |  : null terminated string or pointer to object        |
	 *  ---------------------------------------------------------
	 *  |  stack												|
	 *  |														|
	 *  ---------------------------------------------------------
	 *  | user mode accessible thread struct (uthread)			|
	 *  ---------------------------------------------------------
	 *
	 *  ============== prepare per thread user mode accessible struct ==============
	 *  1. located in stack bottom for check stack integrity
	 */
	tch_thread_uheader* uthread = (tch_thread_uheader*) (mmp->stk_region->poff << CONFIG_PAGE_SHIFT);
	thread->uthread = uthread;
	thread->uthread->cache = (wt_cache_t*) &uthread[1]; 			// cache struct is made at bottom of user stack

	/***
	 *  ================= Prepare Process Argument in topper most area of stack =====================
	 *  Argument Type 1 : Null Terminated strings which is meant for command line argument from shell execution
	 *  Argument Type 2 : Pointer referencing to an object is meant for typical thread execution
	 */
	char* argv = (char*) (((size_t) mmp->stk_region->poff + mmp->stk_region->psz) >> CONFIG_PAGE_SHIFT);
	argv = argv - proc_header->argv_sz;
	if (proc_header->argv_sz > 0) {												// if process argument is null terminated strings,
		memcpy(argv, proc_header->argv, sizeof(char) * proc_header->argv_sz);   // copy them into stack top area
		thread->uthread->t_arg = argv;
	}else {																		// if process argument is just pointer to another object
		thread->uthread->t_arg = proc_header->argv;							// just copy refernece
	}
	mmp->estk = argv;

	/****
	 * ================= setup per process heap ====================================
	 * 1. allocate heap region from init_segment
	 * 2. create heap control block at the begining of heap region
	 * 3. add mapping of this region
	 */
	if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD) {
		if(proc_header->req_heapsz < CONFIG_HEAP_SIZE)
			proc_header->req_heapsz = CONFIG_HEAP_SIZE;
		if(!tch_segmentAllocRegion(0,mmp->heap_region,proc_header->req_heapsz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_DYNAMIC))){
			tch_segmentFreeRegion(mmp->stk_region);
			kfree(regions);
			kfree(mmp->pgd);
			kfree(mmp->dynamic);
			return FALSE;
		}

		struct user_heap* proc_heap = (struct user_heap*) (mmp->heap_region->poff << CONFIG_PAGE_SHIFT);
		paddr_t sheap = (paddr_t) &proc_heap[1];
		paddr_t eheap = (paddr_t) ((mmp->heap_region->poff + mmp->heap_region->psz) << CONFIG_PAGE_SHIFT);

		wt_initRoot(&proc_heap->heap_root);
		wt_initNode(&proc_heap->heap_node,sheap,((size_t) eheap - (size_t) sheap));
		wt_addNode(&proc_heap->heap_root,&proc_heap->heap_node);
		wt_initCache(thread->uthread->cache,CONFIG_MAX_CACHE_SIZE);

		tch_port_addPageEntry(mmp->pgd,mmp->heap_region->poff,mmp->heap_region->flags);
		mmp->dynamic->heap = &proc_heap->heap_root;
	}else {
		thread->uthread->heap = thread->parent->uthread->heap;
		wt_initCache(thread->uthread->cache,CONFIG_MAX_CACHE_SIZE);
	}

	thread->uthread->heap = thread->mm.dynamic->heap;
	thread->uthread->condv = thread->mm.dynamic->condv;
	thread->uthread->mtx = thread->mm.dynamic->mtx;
	thread->uthread = uthread;
	thread->uthread->kthread = thread;
	thread->uthread->fn = proc_header->entry;
	thread->uthread->kRet = tchOK;
	thread->uthread->uobjs = NULL;

	return TRUE;

}

int tch_mmProcClean(tch_thread_kheader* thread){
	if(!thread)
		KERNEL_PANIC("tch_mm.c","thread clean-up fail : null reference");
	struct tch_mm* mmp = &thread->mm;
}



uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl){

	struct mem_segment* segment;
	int seg_id;
	struct section_descriptor** section = &mdesc_tbl[0];		// firts section descriptor should be normal
	if(((*section)->flags & SEGMENT_MSK) != SEGMENT_NORMAL)
		KERNEL_PANIC("tch_mm.c","invalid section descriptor table");

	memset(&init_mm,0,sizeof(struct tch_mm));
	current_mm = &init_mm;
	tch_initSegment(*section);			// initialize segment manager and kernel dyanmic memory manager

	// register segments
	uint32_t* ekstk = NULL;
	section = &mdesc_tbl[1];
	do {
		seg_id = tch_segmentRegister(*section);
		tch_mapSegment(&init_mm,seg_id);
		if(((*section)->flags & SECTION_MSK) == SECTION_STACK)
			ekstk = (*section)->end;
		section++;
	} while (*section);

	return ekstk; // return top address of kernel stack
}


void tch_kernelOnMemFault(paddr_t pa, int fault){
	struct mem_region* region = tch_segmentGetRegionFromPtr(pa);
	if(perm_is_only_priv(region->flags) && !perm_is_public(region->flags) && region->owner != &tch_currentThread->kthread->mm){
		//kill thread
		tch_schedDestroy(tch_currentThread,tchErrorIllegalAccess);
	}

	if(!tch_port_addPageEntry(((struct tch_mm*) &tch_currentThread->kthread->mm)->pgd, (region->poff << CONFIG_PAGE_SHIFT),get_permission(region->flags))) {			// add to table
		tch_schedDestroy(tch_currentThread,tchErrorIllegalAccess);																// already in table?
	}
}


static uint32_t* init_mmProcStack(struct tch_mm* mmp,struct mem_region* stkregion,size_t stksz){
	if(!stksz)										// stack size should not 0
		stksz = CONFIG_THREAD_MIN_STACK;
	if(!(tch_segmentAllocRegion(0,stkregion,stksz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_STACK)) > 0)){
		return NULL;
	}
	tch_port_addPageEntry(mmp->pgd, stkregion->poff,stkregion->flags);
	return (uint32_t*) (stkregion->poff << CONFIG_PAGE_SHIFT);
}


tchStatus __tch_noop_destr(tch_kobj* obj){return tchOK; }


