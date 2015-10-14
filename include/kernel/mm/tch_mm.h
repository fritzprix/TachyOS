/*
 * tch_mm.h
 *
 *  Created on: 2015. 6. 6.
 *      Author: innocentevil
 */

#ifndef TCH_MM_H_
#define TCH_MM_H_

#include "tch_ptypes.h"

#include "kernel/tch_ktypes.h"
#include "kernel/tch_loader.h"
#include "kernel/tch_kconfig.h"
#include "kernel/mm/wtmalloc.h"
#include "kernel/util/cdsl_rbtree.h"
#include "kernel/util/cdsl_slist.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * tachyos aims to allowing multiple program to run simultaneously without any interference between processes
 * or any harm to system stability. so there must be protection unit at the minumum in hardware. kernel itself dosen't has
 * any limitation (I believe) to support low cost 8-bit microcontrollers though, there are many alternatives which is more
 * appropriately designed to those kinds of low cost things.
 *
 *
 * mmu-less architecture (mpu supported)
 * 	. kernel operates in privilidged mode which is able to access all memory directly
 * mmu suppoted architecture
 * 	. kernel operates in privilidged mode though, it can not directly access all address
 * 	. kernel has to prepare its own page table before enabling paging hardware
 *
 * 	1. how memory is virtualized in each hardware architecture respectively.
 * 	- mpu based hardware (most of low(not extremely low....) cost system)
 * 	 . make decision whether accessed memory address is valid or not in memory fault (by cpu)
 * 	 . if it is valid address situation (mpu) update memory permission (protection) table in mpu configuration registers as soon as possible
 * 	   . if all the permission table entry is occupied (arm v7m mpu supports only 8 protection region..), drop least-likely to be used entry, and put new one.
 * 	- mmu based hardware (most of full featured processor like arm v7a or equivalent of.. which includes hardware paging unit )
 * 	 . make decision whether accessed memory address is valid or not (by mmu)
 * 	   .if it is valid mmu translate it to physical address and read or write data into memory (or cache)
 * 	   .if it is not valid address, mmu raises page fault when...
 * 	   	. page entry is valid but it's not present in main memory (swapped or not yet loaded from file)
 * 	   	. page entry is not valid, it's considered as error case, typically the task is signaled to terminate(SIGSEGV)
 *
 * 	2. initialization process by kernel
 * 	 - mpu based hardware
 * 	   . kernel has privilidged permission all the memory address space so initialization is simpler than mmu case
 * 	   . kernel initialize large dynamic memory pool for whole system
 * 	   . Dedicated dynamic memory for kernel is initialized (only privilidged access is permitted)
 * 	   . Shareable dynamic memory for communicating between user & kernel mode but is not writable from user (uni-directional) is initialized
 * 	   . set mpu register to enable
 * 	 - mmu based hardware
 * 	   .
 * 	3. initailization process by normal process
 * 	 - mpu based hardware
 * 	    . kernel create task structure (tch_thread_kheader) from kernel heap and initialize it.
 * 	    . kernel allocate pages (mem_region) where program is to be loaded(text / sdata / bss / stack).
 * 	    . kernel allocate pages for user process heap.
 * 	    . initialize task memory mapping table.
 *
 */

#define PAGE_SIZE		 			(1 << CONFIG_PAGE_SHIFT)
#define PAGE_MASK					(~(PAGE_SIZE - 1))

#define SEGMENT_NORMAL				((uint32_t) 0)
#define SEGMENT_KERNEL				((uint32_t) 1)
#define SEGMENT_UACCESS				((uint32_t) 2)
#define SEGMENT_DEVICE				((uint32_t) 3)

#define SEGMENT_MSK				(SEGMENT_KERNEL | SEGMENT_NORMAL | SEGMENT_DEVICE | SEGMENT_UACCESS)

#define SECTION_UTEXT			((uint32_t) 0 << 3)
#define SECTION_URODATA			((uint32_t) 1 << 3)
#define SECTION_TEXT			((uint32_t) 2 << 3)
#define SECTION_DATA			((uint32_t) 3 << 3)
#define SECTION_STACK			((uint32_t) 4 << 3)
#define SECTION_DYNAMIC			((uint32_t) 5 << 3)

#define SECTION_MSK				((uint32_t) 7 << 3)

#define get_section(flag)		(flag & SECTION_MSK)

#define CACHE_WRITE_THROUGH				((uint32_t) 1 << 6)
#define CACHE_WRITE_BACK_WA				((uint32_t) 2 << 6)
#define CACHE_WRITE_BACK_NWA			((uint32_t) 3 << 6)
#define CACHE_BYPASS					((uint32_t) 4 << 6)

#define CACHE_POLICY_MSK				(CACHE_WRITE_THROUGH | CACHE_WRITE_BACK_WA | CACHE_WRITE_BACK_NWA | CACHE_BYPASS)
#define get_cachepol(flag)				(flag & CACHE_POLICY_MSK)

#define SHAREABLE_MSK					((uint32_t) 1 << 9 )
#define get_shareability(flag)			(flag & SHAREABLE_MSK)

#define PERM_BASE				((uint32_t) SHAREABLE_MSK << 1)
#define PERM_KERNEL_RD			((uint32_t) PERM_BASE << 0)
#define PERM_KERNEL_WR			((uint32_t) PERM_BASE << 1)
#define PERM_KERNEL_XC			((uint32_t) PERM_BASE << 2)
#define PERM_KERNEL_ALL			(PERM_KERNEL_RD | PERM_KERNEL_WR | PERM_KERNEL_XC)

#define PERM_OWNER_RD			((uint32_t) PERM_BASE << 3)
#define PERM_OWNER_WR			((uint32_t) PERM_BASE << 4)
#define PERM_OWNER_XC			((uint32_t) PERM_BASE << 5)
#define PERM_OWNER_ALL			(PERM_OWNER_RD | PERM_OWNER_WR | PERM_OWNER_XC)

#define PERM_OTHER_RD			((uint32_t) PERM_BASE << 6)
#define PERM_OTHER_WR			((uint32_t) PERM_BASE << 7)
#define PERM_OTHER_XC			((uint32_t) PERM_BASE << 8)
#define PERM_OTHER_ALL			(PERM_OTHER_RD | PERM_OTHER_WR | PERM_OTHER_XC)

#define PERM_MSK				(PERM_KERNEL_ALL | PERM_OWNER_ALL | PERM_OTHER_ALL)

#define perm_is_only_priv(flags)		(!(flags & (PERM_OWNER_ALL || PERM_OTHER_ALL)))
#define perm_is_public(flags)			(((flags & PERM_OTHER_ALL) == PERM_OTHER_ALL) && (flags & SHAREABLE_MSK))

#define clr_permission(flag)			(flag &= ~PERM_MSK)
#define get_permission(flag)			(flag & PERM_MSK)

#define set_permission(flag,perm) 		 do {\
	clr_permission(flag);\
	flag |= perm;\
}while(0)

#define MEMTYPE_INROM 			((uint32_t) PERM_MSK + PERM_BASE)
#define MEMTYPE_EXROM			((uint32_t) MEMTYPE_INROM * 2)
#define MEMTYPE_INRAM			((uint32_t) MEMTYPE_INROM * 3)
#define MEMTYPE_EXRAM			((uint32_t) MEMTYPE_INROM * 4)
#define MEMTYPE_MSK				(MEMTYPE_INROM | MEMTYPE_EXROM | MEMTYPE_INRAM | MEMTYPE_EXRAM)

#define get_memtype(flag)		(flag & MEMTYPE_MSK)

#define get_addr_from_page(paddr)	((size_t) paddr << CONFIG_PAGE_SHIFT)

struct section_descriptor {
	uint32_t flags;
	paddr_t start;				///< start address of section in bytes
	paddr_t end;				///< end address of section in bytes
}__attribute__((packed));

typedef struct page_frame page_frame_t;

/**
 *
 */

struct proc_dynamic {
	rb_treeNode_t* mregions;			// region mapping node
	void* heap;
	void* shmem;
	tch_condvId condv;
	tch_mtxId mtx;
};


extern const struct section_descriptor* const default_sections[];

extern int _stext;
extern int _etext;
extern int _initv_begin;
extern int _initv_end;
extern int _exitv_begin;
extern int _exitv_end;
extern int _sdata;
extern int _edata;
extern int _sbss;
extern int _ebss;
extern int _skheap;
extern int _ekheap;
extern int _sstack;
extern int _estack;
extern int _utext_begin;
extern int _utext_end;
extern int _surox;
extern int _eurox;

extern struct tch_mm init_mm;
extern volatile struct tch_mm* current_mm;

extern BOOL tch_mmProcInit(tch_thread_kheader* thread,struct proc_header* proc);
extern BOOL tch_mmProcClean(tch_thread_kheader* thread);
extern uint32_t* tch_kernelMemInit(struct section_descriptor** mdesc_tbl);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_MM_H_ */
