/*
 * tch_kmalloc.h
 *
 *  Created on: Jul 7, 2015
 *      Author: innocentevil
 */

#ifndef TCH_KMALLOC_H_
#define TCH_KMALLOC_H_

#include "tch_mm.h"

extern void tch_initKmalloc(int segid);

extern void* kmalloc(size_t sz);
extern void kfree(void* );


#endif /* TCH_KMALLOC_H_ */
