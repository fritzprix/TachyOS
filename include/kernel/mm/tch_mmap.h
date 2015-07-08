/*
 * tch_mmap.h
 *
 *  Created on: Jul 5, 2015
 *      Author: innocentevil
 */

#ifndef TCH_MMAP_H_
#define TCH_MMAP_H_

#include "tch_mm.h"
#include "tch_segment.h"

extern int tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg);
extern int tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg);
extern BOOL tch_isValidAddress(struct tch_mm* mm, paddr_t addr);


#endif /* TCH_MMAP_H_ */
