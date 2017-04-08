/*
 * tch_nsegment.h
 *
 *  Created on: Oct 16, 2016
 *      Author: innocentevil
 */

#ifndef INCLUDE_KERNEL_MM_TCH_NSEGMENT_H_
#define INCLUDE_KERNEL_MM_TCH_NSEGMENT_H_

#include "tch_mm.h"
#include "tch_kernel.h"

#include "cdsl_rbtree.h"
#include "wtree.h"

// segment is abstract memory area that can be dynamically allocated
// -

struct mem_nsegment {
	rbtreeRoot_t         reg_root;           //  segment로부터 할당된 region들을 관리하기 위한 red black tree, region의 page offset을 key 값으로 같는다.
	rbtreeNode_t         addr_rbn;           //  전역적으로 segment를 lookup 하기 위한 red black tree node로 segment의 page offset을 key 값으로 한다.
	rbtreeNode_t         id_rbn;             //  addr_rbn과 같은 목적을 가지고 있다. 단, segment id값을 key값으로 갖는다.
	uint32_t              poff;               //  segment의 시작 page offset
	uint32_t              psize;              //  segment의 크기 in page
	uint32_t              flags;              //  segment의 메모리 속성 (종류, 용도)를 지정. section_descriptor로부터 상속받게됨
	wtreeRoot_t           reg_pool;           //  segment의 할당을 관리 하기위한 allocator node
};


struct mem_nregion {
	rbtreeNode_t         reg_rbn;            //  segment의 reg_root에 추가하기 위한 red-black tree node 구조체
	rbtreeNode_t         map_rbn;            //  thread control block에 추가하기 위한 red-black tree node 구조체
	struct tch_mm        *owner;              //  할당된 root thread (process)의 control block의 mm에 대한 참조
	uint32_t              poff;               //  region의 page offset
	uint32_t              psize;              //  region의 page size
	uint32_t              flags;              //  region의 속성
};

// init_segment는 segment management를 위한 자료 구조를 초기화 시키고 kernel의 memory allocator를 초기화 한다.
extern int tch_init_segment(const struct section_descriptor* const init_section);
extern int tch_register_section(const struct section_descriptor* const section);
extern tchStatus tch_allocate_region(struct mem_nregion* region, const size_t sz, uint32_t flags);
extern tchStatus tch_free_region(const struct mem_nregion* region);
extern tchStatus tch_map_region(struct tch_mm* mm, struct mem_nregion* region);
extern tchStatus tch_unmap_region(struct mem_nregion* region);


#endif /* INCLUDE_KERNEL_MM_TCH_NSEGMENT_H_ */
