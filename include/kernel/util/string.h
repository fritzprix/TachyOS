/*
 * string.h
 *
 *  Created on: 2015. 10. 3.
 *      Author: innocentevil
 */

#ifndef STRING_H_
#define STRING_H_

#include "tch_types.h"
#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif


extern void mset(void* dst,int v,size_t sz);
extern void mcpy(void* dst,const void* src,size_t n);
extern int mcmp(const void* s1,const void* s2,size_t n);
extern char* strchar(const char* s,int c);
extern size_t strcopy(char* dst,char* src);
extern size_t ftostr(float val,char* str,int trunc);
extern size_t itostr(int val,char* str,int radix);
extern size_t vformat(char* dst,const char* fmt,va_list args);
extern size_t format(char* dest,const char* fmt,...);

#if defined(__cplusplus)
}
#endif



#endif /* STRING_H_ */
