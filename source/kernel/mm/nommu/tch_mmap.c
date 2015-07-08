/*
 * tch_mmap.c
 *
 *  Created on: Jul 5, 2015
 *      Author: innocentevil
 */


#include "tch_mmap.h"


int tch_mapRegion(struct tch_mm* mm,struct mem_region* mreg){
	if(!mm || !mreg)
		return -1;
}

int tch_unmapRegion(struct tch_mm* mm,struct mem_region* mreg){

}

BOOL tch_isValidAddress(struct tch_mm* mm, paddr_t addr){

}
