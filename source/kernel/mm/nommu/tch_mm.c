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




#ifndef CONFIG_NR_KERNEL_SEG
#define CONFIG_NR_KERNEL_SEG	5		// segment 0 dynamic /  segment 1 kernel text / segment 2 data / segment 3 sdata /  segment 4 kernel stack
#endif

struct user_heap {
	wt_heapRoot_t		heap_root;
	wt_heapNode_t		heap_node;
};

/**
 *  NOTE : the order of section should not change
 */

const struct section_descriptor __default_sections[] = {
		{		// kernel dynamic section
				.flags = (MEMTYPE_INRAM | SEGMENT_NORMAL | SECTION_DYNAMIC),
				.start = &_skheap,
				.end = &_ekheap
		},
		{
				.flags = (MEMTYPE_INROM | SEGMENT_KERNEL | SECTION_UTEXT),
				.start = &_utext_begin,
				.end = &_utext_end
		},
		{
				.flags = (MEMTYPE_INROM | SEGMENT_KERNEL | SECTION_URODATA),
				.start = &_surox,
				.end = &_eurox
		},
		{
				// kernel text section
				.flags = (MEMTYPE_INROM | SEGMENT_KERNEL | SECTION_TEXT),
				.start = &_stext,
				.end = &_etext
		},
		{		// kernel bss section (zero filled data)
				.flags = (MEMTYPE_INRAM | SEGMENT_KERNEL | SECTION_DATA),
				.start = &_sbss,
				.end = &_ebss
		},
		{		// kernel data section (initialized to specified value)
				.flags = (MEMTYPE_INRAM | SEGMENT_KERNEL | SECTION_DATA),
				.start = &_sdata,
				.end = &_edata
		},
		{		// kernel stack
				.flags = (MEMTYPE_INRAM | SEGMENT_KERNEL | SECTION_STACK),
				.start = &_sstack,
				.end = &_estack
		}
};


const struct __attribute__((section(".data"))) section_descriptor* const default_sections[] = {
		&__default_sections[0],
		&__default_sections[1],
		&__default_sections[2],
		&__default_sections[3],
		&__default_sections[4],
		&__default_sections[5],
		&__default_sections[6],
		NULL
};


volatile struct tch_mm*		current_mm;
struct tch_mm				init_mm;


static uint32_t* init_mmProcStack(struct tch_mm* mmp,struct mem_region* stkregion,size_t stksz);

BOOL tch_mmProcInit(tch_thread_kheader* thread,struct proc_header* proc_header)
{
	/**
	 *  add mapping to region containing process binary image
	 */
	wt_heapRoot_t* heap_root;
	wt_heapNode_t* init_heap;
	tch_mtxCb* mtx;
	tch_condvCb* condv;
	wt_cache_t* cache;
	struct tch_mm* mmp = &thread->mm;
	mset(mmp,0,sizeof(struct tch_mm));
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
	if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD)
	{
		mmp->pgd = tch_port_allocPageDirectory(kmalloc);
		mmp->dynamic = (struct proc_dynamic*) kmalloc(sizeof(struct proc_dynamic));
		mtx = (tch_mtxCb*) kmalloc(sizeof(tch_mtxCb));
		condv = (tch_condvCb*) kmalloc(sizeof(tch_condvCb));
		if(!mmp->dynamic || !mmp->pgd || !mtx || !condv)
		{
			kfree(condv);
			kfree(mtx);
			kfree(mmp->pgd);
			kfree(mmp->dynamic);
			return FALSE;
		}

		mmp->flags |= ROOT;
		mmp->dynamic->mtx = mtx;
		mmp->dynamic->condv = condv;
		if(proc_header->flag & PROCTYPE_DYNAMIC)
		{
			// dynamic loaded process
			mmp->text_region = proc_header->text_region;
			mmp->bss_region = proc_header->bss_region;
			mmp->data_region = proc_header->data_region;
			mmp->flags |= DYN;
		}
		else
		{
			// text/ bss / data section of static process are assumed to be part of kernel binary image
			mmp->text_region = NULL;
			mmp->bss_region = NULL;
			mmp->data_region = NULL;
		}

		mmp->dynamic->mregions = NULL;
		tch_mutexInit(mmp->dynamic->mtx);
		tch_condvInit(mmp->dynamic->condv);
	}
	else
	{
		struct tch_mm* parent_mm = &current->kthread->parent->mm;
		mcpy(mmp,parent_mm,sizeof(struct tch_mm));
		mmp->flags = 0;
		mmp->pgd = tch_port_allocPageDirectory(kmalloc);
		if(!mmp->pgd)
			return FALSE;
		mmp->dynamic = parent_mm->dynamic;
		cdsl_dlistInit(&mmp->kobj_list);
		cdsl_dlistInit(&mmp->alc_list);
		cdsl_dlistInit(&mmp->shm_list);
	}

	if(mmp->text_region && mmp->bss_region && mmp->data_region){
		tch_port_addPageEntry(mmp->pgd, mmp->text_region->poff, mmp->text_region->flags);
		tch_port_addPageEntry(mmp->pgd, mmp->bss_region->poff, mmp->bss_region->flags);
		tch_port_addPageEntry(mmp->pgd, mmp->data_region->poff, mmp->data_region->flags);
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

	// try allocate stack region from dynamic segment
	if(!init_mmProcStack(mmp,mmp->stk_region,proc_header->req_stksz))
	{
		// if stack allocation fail, clean up and return
		if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD)
		{
			tch_condvDeinit(condv);
			tch_mutexDeinit(mtx);
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
	tch_thread_uheader* uthread = (tch_thread_uheader*) (mmp->stk_region->poff << PAGE_OFFSET);
	thread->uthread = uthread;
	thread->uthread->cache = (wt_cache_t*) &uthread[1]; 			// cache struct is made at bottom of user stack

	/***
	 *  ================= Prepare Process Argument in topper most area of stack =====================
	 *  Argument Type 1 : Null Terminated strings which is meant for command line argument from shell execution
	 *  Argument Type 2 : Pointer referencing to an object is meant for typical thread execution
	 */
	char* argv = (char*) (((size_t) mmp->stk_region->poff + mmp->stk_region->psz) << PAGE_OFFSET);
	argv = (char*) ((int)(argv - proc_header->argv_sz) & ~0xF);
	if (proc_header->argv_sz > 0)
	{												// if process argument is null terminated strings,
		mcpy(argv, proc_header->argv, sizeof(char) * proc_header->argv_sz);   // copy them into stack top area
		thread->uthread->t_arg = argv;
	}
	else
	{																		// if process argument is just pointer to another object
		thread->uthread->t_arg = proc_header->argv;				    			// just copy refernece
	}
	mmp->estk = argv;			// end of stack

	/****
	 * ================= setup per process heap ====================================
	 * 1. allocate heap region from init_segment
	 * 2. create heap control block at the begining of heap region
	 * 3. add mapping of this region
	 */
	if((proc_header->flag & HEADER_TYPE_MSK) == HEADER_ROOT_THREAD) {
		if(proc_header->req_heapsz < HEAP_SIZE)
			proc_header->req_heapsz = HEAP_SIZE;
		if(!tch_segmentAllocRegion(0,mmp->heap_region,proc_header->req_heapsz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_DYNAMIC))){
			tch_segmentFreeRegion(mmp->stk_region);
			kfree(regions);
			kfree(mmp->pgd);
			kfree(mmp->dynamic);
			return FALSE;
		}

		struct user_heap* proc_heap = (struct user_heap*) (mmp->heap_region->poff << PAGE_OFFSET);
		paddr_t sheap = (paddr_t) &proc_heap[1];
		paddr_t eheap = (paddr_t) ((mmp->heap_region->poff + mmp->heap_region->psz) << PAGE_OFFSET);

		wt_initRoot(&proc_heap->heap_root);
		wt_initNode(&proc_heap->heap_node,sheap,((size_t) eheap - (size_t) sheap));
		wt_addNode(&proc_heap->heap_root,&proc_heap->heap_node);
		wt_initCache(thread->uthread->cache,MAX_CACHE_SIZE);

		tch_port_addPageEntry(mmp->pgd,mmp->heap_region->poff,mmp->heap_region->flags);
		mmp->dynamic->heap = &proc_heap->heap_root;
	}else {
		thread->uthread->heap = thread->parent->uthread->heap;
		wt_initCache(thread->uthread->cache,MAX_CACHE_SIZE);
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

BOOL tch_mmProcClean(tch_thread_kheader* thread){
	if(!thread)
		KERNEL_PANIC("thread clean-up fail : null reference");
	struct tch_mm* mmp = &thread->mm;
	wt_cacheFlush(thread->uthread->heap,thread->uthread->cache);		// return cached chunk to per process heap
	tch_segmentFreeRegion(mmp->stk_region);								// return user stack region
	if((mmp->flags & ROOT) == ROOT){									// if current thread is root thread in thread group
		tch_segmentFreeRegion(mmp->heap_region);						// free heap region
		tch_mutexDeinit(mmp->dynamic->mtx);
		tch_condvDeinit(mmp->dynamic->condv);
		kfree(mmp->dynamic->mtx);
		kfree(mmp->dynamic->condv);
		kfree(mmp->dynamic);											// free memory area for dynamic struct
	}

	if((mmp->flags & DYN) == DYN) {										// if current thread is dynamically loaded at first
		tch_segmentFreeRegion(mmp->bss_region);							// free bss region
		tch_segmentFreeRegion(mmp->data_region);						// free data region
		tch_segmentFreeRegion(mmp->text_region);						// text region
	}

	kfree(mmp->heap_region);											// return 2 mem_region struct pointer to kernel heap
	kfree(mmp->pgd);													// free page directory

	return TRUE;
}



uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl){

	struct mem_segment* segment;
	int seg_id;
	struct section_descriptor** section = &mdesc_tbl[0];		// firts section descriptor should be normal
	if(((*section)->flags & SEGMENT_MSK) != SEGMENT_NORMAL)
		KERNEL_PANIC("invalid section descriptor table");

	mset(&init_mm,0,sizeof(struct tch_mm));
	current_mm = &init_mm;
	tch_initSegment(*section);			// initialize segment manager and kernel dyanmic memory manager

	// register segments
	uint32_t* ekstk = NULL;
	section = &mdesc_tbl[1];
	do {
		seg_id = tch_segmentRegister(*section);
		tch_mapSegment(&init_mm,seg_id);
		if(((*section)->flags & SECTION_MSK) == SECTION_STACK)			// find kernel stack end (initial kernel stack pointer)
			ekstk = (*section)->end;
		section++;
	} while (*section);

	return ekstk; // return top address of kernel stack
}


void tch_kernel_onMemFault(paddr_t pa, int fault,int spec){
	struct mem_region* region = tch_segmentGetRegionFromPtr(pa);
	if(perm_is_only_priv(region->flags) && !perm_is_public(region->flags) && region->owner != &current->kthread->mm){
		//kill thread
		tch_thread_exit(current,tchErrorIllegalAccess);
	}

	if(!tch_port_addPageEntry(((struct tch_mm*) &current->kthread->mm)->pgd, (region->poff << PAGE_OFFSET),get_permission(region->flags))) {			// add to table
		tch_thread_exit(current,tchErrorIllegalAccess);																// already in table?
	}
}


static uint32_t* init_mmProcStack(struct tch_mm* mmp,struct mem_region* stkregion,size_t stksz){
	if(!stksz)										// stack size should not 0
		stksz = USER_MIN_STACK;
	if(!(tch_segmentAllocRegion(0,stkregion,stksz,(PERM_KERNEL_ALL | PERM_OWNER_ALL | SECTION_STACK)) > 0)){
		return NULL;
	}
	tch_port_addPageEntry(mmp->pgd, stkregion->poff,stkregion->flags);
	return (uint32_t*) (stkregion->poff << PAGE_OFFSET);
}



