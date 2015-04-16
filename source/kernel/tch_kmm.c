/*
 * tch_palloc.c
 *
 *  Created on: 2015. 3. 21.
 *      Author: innocentevil
 */
#include "tch_kernel.h"
#include "tch_mtx.h"
#include "tch_kmm.h"

#define KMM_VALIDATION_KEY			((uint32_t) 0x3A3A)
#define SHMEM_VALIDATION_KEY		((uint32_t) 0x3D3A)
#define KHEAP_VALIDATION_KEY		((uint32_t) 0xD3A3)
#define PAGE_KEY					((uint32_t) 0x7A7A)

#define SHMEM_VALIDATE(shm)			do {\
	((struct tch_shmem_mgr_handle*)shm)->sh_status = (((uint32_t) shm ^ SHMEM_VALIDATION_KEY) & 0xFFFF);\
}while(0)

#define SHMEM_INVALIDATE(shm) 		do {\
	((struct tch_shmem_mgr_handle*)shm)->sh_status &= ~0xFFFF;\
}while(0)

#define SHMEM_ISVALID(shm)			((((struct tch_shmem_mgr_handle*)shm)->sh_status & 0xFFFF) == (((uint32_t) shm ^ SHMEM_VALIDATION_KEY) & 0xFFFF))


#define KHEAP_VALIDATE(kheap) 		do {\
	((struct tch_kmem_mgr_handle*) kheap)->k_status = (((uint32_t) kheap ^ KHEAP_VALIDATION_KEY) & 0xFFFF);\
}while(0)

#define KHEAP_IVALIDATE(kheap) 		do {\
	((struct tch_kmem_mgr_handle*) kheap)->k_status = (((uint32_t) kheap ^ KHEAP_VALIDATION_KEY) & 0xFFFF);\
}while(0)

#define KHEAP_ISVALID(kheap)		((((struct tch_kmem_mgr_handle*) kheap)->k_status & 0xFFFF) == (((uint32_t) kheap ^ KHEAP_VALIDATION_KEY) & 0xFFFF))


#define PAGE_MGR_VALIDATE(kmm)			do {\
	((struct tch_page_mgr_handle*)kmm)->p_status = (((uint32_t) kmm ^ KMM_VALIDATION_KEY) & 0xFFFF);\
}while(0)

#define PAGE_MGR_INVALIDATE(kmm) 		do {\
	((struct tch_page_mgr_handle*)kmm)->p_status &= ~0xFFFF;\
}while(0)

#define PAGE_MGR_ISVALID(kmm)			((((struct tch_page_mgr_handle*)kmm)->p_status & 0xFFFF) == (((uint32_t) kmm ^ KMM_VALIDATION_KEY) & 0xFFFF))

#define PAGE_VALIDATE(pheader) 		do {\
	pheader->p_status = (((uint32_t) pheader ^ PAGE_KEY) & 0xFFFF);\
}while(0)

#define PAGE_INVALIDATE(pheader)	do {\
	pheader->p_status &= ~0xFFFF;\
}while(0)

#define PAGE_ISVALID(pheader) 		((((struct tch_page_header*)pheader)->p_status & 0xFFFF) == (((uint32_t) pheader ^ PAGE_KEY) & 0xFFFF))


#define PAGE_SET_SR_CNT(pheader,cnt) do {\
	pheader->p_status &= ~0xF0000;\
	pheader->p_status |= cnt < 16;\
}while(0)

#define PAGE_GET_SR_CNT(pheader) 	((((tch_page_header*) pheader)->p_status >> 16) ^ 0xF)

typedef struct tch_uheap_handle_t tch_uheap_handle;

struct tch_uheap_handle_t {
	tch_memId		u_mem;
	tch_lnode		u_alc;
}__attribute__((packed));

struct tch_page_mgr_handle {
	tch_memId 		p_mem;
	tch_lnode		p_alc;
	tch_lnode		p_phle;
	uint32_t 		p_status;
	int 			p_base_mpermId;
}__attribute__((packed));

typedef struct tch_pageSubRegion {
	uint8_t* 		addr;
	size_t 			sz_shoffset;
	int 			permId;
}tch_pageSubRegion;

struct tch_page_header {
	tch_lnode				p_le;
	tch_pageAccessibility 	p_acc;
	tch_threadId 			p_owner;
	uint8_t					p_regCnt;
	size_t					p_size;
	tch_pageSubRegion		p_reg[4];			// each page has 4 sub region
	uint32_t				p_status;
}__attribute__((aligned(8)));


struct tch_shmem_mgr_handle {
	tch_pageId 				sh_pmPageId;
	tch_pageId				sh_upmPageId;
	tch_memId				sh_pmId;
	tch_memId				sh_upmId;
	uint32_t				sh_status;
}__attribute__((packed));

struct tch_kmem_mgr_handle {
	tch_pageId				k_pgId;
	tch_memId				k_memId;
	uint32_t				k_status;
	tch_lnode				k_alc_le;
}__attribute__((packed));

struct tch_shmem_header {
	BOOL 					sh_prot;
}__attribute__((aligned(8)));


static struct tch_page_mgr_handle PageManagerHandle = {0};
static struct tch_shmem_mgr_handle ShareableMemHandle = {0};
static struct tch_kmem_mgr_handle KernelHeapHandle = {0};

static void* tch_userAlloc(size_t sz);
static void tch_userFree(void* mchunk);
static uint32_t tch_userAvail(tch_memId mh);

const tch_mem_ix UserHeapInterface = {
		.alloc = tch_userAlloc,
		.free = tch_userFree,
		.available = tch_userAvail
};

const tch_mem_ix* uMem = &UserHeapInterface;
/**
 *
 */
tchStatus tchk_pageInit(void* kmem_base,uint32_t msz){
	PageManagerHandle.p_mem = (tch_memId) tch_memInit(kmem_base,msz,FALSE);
	if(PageManagerHandle.p_mem == NULL)
		return tchErrorParameter;
	tch_listInit(&PageManagerHandle.p_alc);	// initialize page allocation list
	PageManagerHandle.p_base_mpermId = tch_port_setMemPermission(kmem_base,msz,MEM_PRIV_READ_PERMISSION | MEM_PRIV_WRITE_PERMISSION);  // kernel heap is protected from unprivilidged access by default
	PAGE_MGR_VALIDATE(&PageManagerHandle);
	return tchOK;
}

tchStatus tchk_shareableMemInit(uint32_t msz){
	uint8_t* mem = NULL;
	uint32_t psz = 0;
	if(!PAGE_MGR_ISVALID(&PageManagerHandle))
		return tchErrorOS;
	ShareableMemHandle.sh_pmPageId = tchk_pageRequest(NULL,msz,TCH_PAGE_ACCESSIBILITY_SHARED_PROTECTED);		// allocate protected page
	ShareableMemHandle.sh_upmPageId = tchk_pageRequest(NULL,msz,TCH_PAGE_ACCESSIBILITY_SHARED);				// allocate unprotected page

	tchk_mapPage(ShareableMemHandle.sh_pmPageId); 				// apply page mapping (into MPU)
	tchk_mapPage(ShareableMemHandle.sh_upmPageId);

	mem = tchk_getPageAddress(ShareableMemHandle.sh_pmPageId);	// get base address of page
	psz = tchk_getPageSize(ShareableMemHandle.sh_pmPageId);			// get size of page  (should be power of two)
	ShareableMemHandle.sh_pmId = (tch_memId) tch_memInit(mem,psz,FALSE);		// initialize allocator for read/writable sharable memory

	mem = tchk_getPageAddress(ShareableMemHandle.sh_upmPageId);
	psz = tchk_getPageSize(ShareableMemHandle.sh_upmPageId);
	ShareableMemHandle.sh_upmId = (tch_memId) tch_memInit(mem,psz,FALSE);	// initialize allocator for read only sharable memory
	SHMEM_VALIDATE(&ShareableMemHandle);
	return tchOK;
}

tchStatus tchk_kernelHeapInit(uint32_t msz){
	uint8_t* mem = NULL;
	uint32_t psz = 0;
	if(!PAGE_MGR_ISVALID(&PageManagerHandle))
		return tchErrorOS;
	KernelHeapHandle.k_pgId = tchk_pageRequest(NULL,msz,TCH_PAGE_ACCESSIBILITY_KERNEL);
	tchk_mapPage(KernelHeapHandle.k_pgId);
	mem = tchk_getPageAddress(KernelHeapHandle.k_pgId);
	psz = tchk_getPageSize(KernelHeapHandle.k_pgId);
	KernelHeapHandle.k_memId = (tch_memId) tch_memInit(mem,psz,FALSE);
	tch_listInit(&KernelHeapHandle.k_alc_le);
	KHEAP_VALIDATE(&KernelHeapHandle);
	return tchOK;
}



tch_pageId tchk_pageRequest(tch_thread_kheader* thread,uint32_t psz,tch_pageAccessibility acc){
	uint8_t shcnt = 0,bcnt = 0,idx = 0;
	uint8_t sh[4] = {0};
	uint32_t psztst = psz + sizeof(struct tch_page_header);
	uint32_t permission = 0;
	uint8_t* mem = NULL;
	struct tch_page_header* pheader = NULL;
	while(psztst && bcnt < 4){
		if(psztst & 0x80000000){
			sh[bcnt++] = 31 - shcnt;
		}
		shcnt++;
		psztst <<= 1;
	}
	psztst = 0;
	while(idx < bcnt){
		psztst |= (1 << sh[idx++]);
	}
	psztst += (1 << sh[bcnt - 1]);

	if(tch_memAvail(PageManagerHandle.p_mem) < psztst)
		return NULL;
	pheader = (struct tch_page_header*) tch_memAlloc(PageManagerHandle.p_mem,psztst,&PageManagerHandle.p_alc);
	if(pheader == NULL)
		return NULL;

	pheader->p_size = psztst - sizeof(struct tch_page_header);
	pheader->p_status &= ~MEM_PERMISSION_MSK;
	pheader->p_regCnt = bcnt;
	mem = (uint8_t*)(pheader + 1);
	while(bcnt--){
		pheader->p_reg[bcnt].sz_shoffset = sh[bcnt];
		pheader->p_reg[bcnt].addr = mem;
		pheader->p_reg[bcnt].permId = 0;
		mem = (uint8_t*) ((uint32_t) mem + (1 << sh[bcnt]));
	}
	pheader->p_owner = NULL;
	pheader->p_acc = acc;

	switch(acc){
	case TCH_PAGE_ACCESSIBILITY_SHARED:
		permission = (MEM_PRIV_READ_PERMISSION | MEM_PRIV_WRITE_PERMISSION | MEM_UNPRIV_READ_PERMISSION | MEM_UNPRIV_WRITE_PERMISSION);
		break;
	case TCH_PAGE_ACCESSIBILITY_SHARED_PROTECTED:
		pheader->p_status |= (MEM_PRIV_READ_PERMISSION | MEM_PRIV_WRITE_PERMISSION | MEM_UNPRIV_READ_PERMISSION);
		break;
	case TCH_PAGE_ACCESSIBILITY_PRIVATE:
		if(!thread){
			tch_memFree(pheader);
			return NULL;
		}
		tch_listPutTail(&thread->t_palc,(tch_lnode*) pheader);
		pheader->p_status |= (MEM_PRIV_READ_PERMISSION | MEM_PRIV_WRITE_PERMISSION | MEM_UNPRIV_READ_PERMISSION | MEM_UNPRIV_WRITE_PERMISSION);
		pheader->p_owner = thread->t_uthread;
		break;
	case TCH_PAGE_ACCESSIBILITY_KERNEL:
		pheader->p_status |= (MEM_PRIV_READ_PERMISSION | MEM_PRIV_WRITE_PERMISSION);
		break;
	}

	PAGE_VALIDATE(pheader);
	return (tch_pageId) pheader;
}

void tchk_pageRelease(tch_pageId pgId){
	if(!pgId)
		return;
	struct tch_page_header* pheader = (struct tch_page_header*) pgId;
	if(!PAGE_ISVALID(pheader))
		return;
	PAGE_INVALIDATE(pheader);
	tch_memFree(PageManagerHandle.p_mem,pgId);
}


tchStatus tchk_mapPage(tch_pageId pgId){
	if(!pgId)
		return tchErrorParameter;
	struct tch_page_header* pheader = (struct tch_page_header*) pgId;
	if(!PAGE_ISVALID(pheader))
		return tchErrorParameter;
	uint8_t idx = 0;
	tch_pageSubRegion* subreg = NULL;
	uint8_t* mem = (uint8_t*) ((uword_t)pheader + sizeof(struct tch_page_header));
	while(idx++ < pheader->p_regCnt){
		subreg = &pheader->p_reg[idx];
		subreg->permId = tch_port_setMemPermission(subreg->addr,subreg->sz_shoffset,(pheader->p_status & MEM_PERMISSION_MSK));
		if(subreg->permId == 0)
			return tchErrorOS;
	}
	return tchOK;
}

tchStatus tchk_unmapPage(tch_pageId pgId){
	if(!pgId)
		return tchErrorParameter;
	struct tch_page_header* pheader  = (struct tch_page_header*) pgId;
	if(!PAGE_ISVALID(pheader))
		return tchErrorParameter;
	uint8_t idx = 0;
	tch_pageSubRegion* subreg = NULL;
	while(idx++ < pheader->p_regCnt){
		subreg = &pheader->p_reg[idx];
		if(!tch_port_clrMemPermission(subreg->permId))
			return tchErrorOS;
	}
	return tchOK;
}

void* tchk_getPageAddress(tch_pageId pgId){
	if(!pgId)
		return NULL;
	struct tch_page_header* pheader = (struct tch_page_header*) pgId;
	if(!PAGE_ISVALID(pheader))
		return NULL;
	return (uint8_t*) ((uint32_t) pheader + sizeof(struct tch_page_header));
}

size_t tchk_getPageSize(tch_pageId pgId){
	if(!pgId)
		return 0;
	struct tch_page_header* pheader = (struct tch_page_header*) pgId;
	if(!PAGE_ISVALID(pheader))
		return 0;
	return pheader->p_size;
}


void* tchk_kernelHeapAlloc(size_t sz){
	if(!sz || !KHEAP_ISVALID(&KernelHeapHandle))
		return NULL;
	return (void*) tch_memAlloc(KernelHeapHandle.k_memId,sz,&KernelHeapHandle.k_alc_le);
}

void tchk_kernelHeapFree(void* km_chnk){
	if(!km_chnk || !KHEAP_ISVALID(&KernelHeapHandle))
		return;
	if(tch_memFree(KernelHeapHandle.k_memId,km_chnk,&KernelHeapHandle.k_alc_le) != tchOK)
		tch_kernel_errorHandler(FALSE,tchErrorOS);
}

uint32_t tchk_kernelHeapAvail(){
	if(!KHEAP_ISVALID(&KernelHeapHandle))
		return 0;
	return tch_memAvail(KernelHeapHandle.k_memId);
}


tchStatus tchk_kernelHeapFreeAll(){
	if(!KHEAP_ISVALID(&KernelHeapHandle))
		return tchErrorOS;
	return tch_memFreeAll(KernelHeapHandle.k_memId,&KernelHeapHandle.k_alc_le,FALSE);
}

void* tchk_shareableMemAlloc(size_t sz,BOOL protection){
	tch_memId mem = NULL;
	tch_lnode* al_le = NULL;
	tch_thread_kheader* kheader = tch_currentThread->t_kthread;
	if(protection){
		mem = ShareableMemHandle.sh_pmId;
		al_le = &kheader->t_pshalc;
	}else{
		mem = ShareableMemHandle.sh_upmId;
		al_le = &kheader->t_upshalc;
	}
	struct tch_shmem_header* header = (struct tch_shmem_header*) tch_memAlloc(mem,sz + sizeof(struct tch_shmem_header),al_le);
	header->sh_prot = protection;
	return (uint8_t*) ((uint32_t) header + sizeof(struct tch_shmem_header));
}

void tchk_shareableMemFree(void* m){
	struct tch_shmem_header* header = (struct tch_shmem_header*) ((uint32_t) m - sizeof(struct tch_shmem_header));
	tch_thread_kheader* kheader = tch_currentThread->t_kthread;

	tch_memId mem = header->sh_prot? ShareableMemHandle.sh_pmId : ShareableMemHandle.sh_upmId;
	tch_lnode* alc_le = header->sh_prot? &kheader->t_pshalc : &kheader->t_upshalc;
	tch_memFree(mem,m,alc_le);
}

uint32_t tchk_shareableMemAvail(BOOL prot){
	tch_memId mem = prot? ShareableMemHandle.sh_pmId : ShareableMemHandle.sh_upmId;
	return tch_memAvail(mem);
}


tchStatus tchk_shareableMemFreeAll(tch_thread_kheader* owner){
	if(!owner || !tchk_threadIsValid(owner->t_uthread) || !SHMEM_ISVALID(&ShareableMemHandle))
		return tchErrorParameter;

	tch_memFreeAll(ShareableMemHandle.sh_pmId,&owner->t_pshalc,TRUE);
	tch_memFreeAll(ShareableMemHandle.sh_upmId,&owner->t_upshalc,TRUE);
	return tchOK;
}

tchStatus tchk_userMemInit(tch_thread_kheader* owner,tch_userMemDef_t* mem_def,BOOL isroot){
	if(!PAGE_MGR_ISVALID(&PageManagerHandle)){
		tch_kernel_errorHandler(FALSE,tchErrorOS);
		return tchErrorOS;			// unreachable code
	}
	if(!owner)
		return tchErrorParameter;
	uint8_t* mem = NULL;
	tch_uheap_handle* heap_handle = NULL;
	uint32_t msz = 0;
	if(isroot){
		/* if user memory space requested from a new root thread
		 * creating context, new page is allocated*/

		msz = mem_def->heap_sz + mem_def->pimg_sz + mem_def->stk_sz + sizeof(tch_thread_uheader);
		owner->t_pgId = tchk_pageRequest(owner,msz,TCH_PAGE_ACCESSIBILITY_PRIVATE);
		if(!owner->t_pgId)
			return tchErrorNoMemory; // page request is not handled
		mem = tchk_getPageAddress(owner->t_pgId);
		msz = tchk_getPageSize(owner->t_pgId);
	}else{
		/* if user memory space requested from any child thread creating context, pre-allocated
		 * memory chunk is used
		 */
		owner->t_pgId = owner->t_parent->t_pgId;
		if(!mem_def->u_mem || !mem_def->u_memsz)
			return tchErrorParameter;
		mem =(uint8_t*)  mem_def->u_mem;
		msz = mem_def->u_memsz;
	}

	owner->t_uthread = (tch_thread_uheader*) mem;	// create user thread contol block in user memory space
	owner->t_uthread->t_kthread = owner;
	owner->t_uthread->t_arg = NULL;
	owner->t_uthread->t_fn = NULL;
	owner->t_uthread->t_name = NULL;
	owner->t_uthread->t_destr = __tch_noop_destr;

	mem = (uint8_t*)((uint32_t) mem + msz);			// move to top address of page
	heap_handle = (tch_uheap_handle*) mem;
	heap_handle--;									// push and make heap handle, and construct heap handle
	if(isroot){
		tch_listInit(&heap_handle->u_alc);
		mem = (uint8_t*) ((uint32_t) heap_handle - mem_def->heap_sz);			// offset heap size
		heap_handle->u_mem = (uint8_t*) tch_memInit(mem,mem_def->heap_sz,TRUE);			// initialize heap
	}else{
		mem = (uint8_t*) heap_handle;
		tch_listInit(&heap_handle->u_alc);															// initailize allocation list
		heap_handle->u_mem = ((tch_uheap_handle*) owner->t_parent->t_uthread->t_heap)->u_mem;		// inherit heap handle from parent
	}

	owner->t_uthread->t_heap = heap_handle;
	owner->t_proc = (uint8_t*)((uint32_t) mem - mem_def->pimg_sz);		// stack will be placed below program image
	return tchOK;
}

tchStatus tchk_userMemFreeAll(tch_thread_kheader* owner){
	if(!owner || !tchk_threadIsValid(owner->t_uthread))
		return tchErrorParameter;
	tch_thread_uheader* uheader = owner->t_uthread;
	tch_uheap_handle* uheap = (tch_uheap_handle*) uheader->t_heap;
	return tch_memFreeAll(uheap->u_mem,&uheap->u_alc,TRUE);
}


static void* tch_userAlloc(size_t sz){
	if(tch_port_isISR() || !tch_currentThread)
		return NULL;
	tch_thread_uheader* uheader = tch_currentThread;
	tch_uheap_handle* heap_handle = (tch_uheap_handle*) uheader->t_heap;
	uint8_t* mem = (uint8_t*) tch_memAlloc(heap_handle->u_mem,sz,&heap_handle->u_alc);
	return mem;
}

static void tch_userFree(void* mchunk){
	if(tch_port_isISR() || !tch_currentThread)
		return;
	tch_uheap_handle* heap_handle = (tch_uheap_handle*) tch_currentThread->t_heap;
	tch_memFree(heap_handle->u_mem,mchunk, &heap_handle->u_alc);
}


static uint32_t tch_userAvail(tch_memId mh){
	if(tch_port_isISR() || !tch_currentThread)
		return 0;
	tch_uheap_handle* heap = (tch_uheap_handle*) tch_currentThread->t_heap;
	return tch_memAvail(heap->u_mem);
}


void* tch_shMemAlloc(size_t sz,BOOL prot){
	if(!sz || !SHMEM_ISVALID(&ShareableMemHandle))
		return NULL;
	if(tch_port_isISR())
		return tchk_shareableMemAlloc(sz,prot);
	return tch_port_enterSv(SV_SHMEM_ALLOC,(uword_t) sz,(uword_t) prot);
}

void tch_shMemFree(void* mchunk){
	if(!mchunk || !SHMEM_ISVALID(&ShareableMemHandle))
		return;
	if(tch_port_isISR())
		return tchk_shareableMemFree(mchunk);
	tch_port_enterSv(SV_SHMEM_FREE,(uword_t) mchunk,0);
}

uint32_t tch_shMemAvali(BOOL prot){
	if(!SHMEM_ISVALID(&ShareableMemHandle))
		return 0;
	if(tch_port_isISR())
		return tchk_shareableMemAvail(prot);
	return tch_port_enterSv(SV_SHMEM_AVAILABLE,prot,0);
}

tchStatus __tch_noop_destr(tch_uobj* obj){return tchOK; }
