/*
 * tch_malloc.h
 *
 *  Created on: 2015. 7. 19.
 *      Author: innocentevil
 */

#ifndef TCH_MALLOC_H_
#define TCH_MALLOC_H_

#include "tch_ktypes.h"


extern void* tch_malloc(size_t sz);
extern void tch_free(void* ptr);
extern size_t tch_avail();


#endif /* TCH_MALLOC_H_ */
