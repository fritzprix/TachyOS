/*
 * string.h
 *
 *  Created on: 2015. 10. 3.
 *      Author: innocentevil
 */

#ifndef STRING_H_
#define STRING_H_

#include "tch_types.h"

#if defined(__cplusplus)
extern "C" {
#endif


extern void memset(void* dst,int v,size_t sz);
extern void memcpy(void* dst,const void* src,size_t n);
extern int memcmp(const void* s1,const void* s2,size_t n);



#if defined(__cplusplus)
}
#endif



#endif /* STRING_H_ */
