/*
 * tch_nclib.h
 *
 *  Created on: 2014. 7. 27.
 *      Author: manics99
 */

#ifndef TCH_NCLIB_H_
#define TCH_NCLIB_H_


#include "tch.h"
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <wctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__cplusplus)
extern "C"{
#endif

struct crt_param {
	struct tch_file* 	__stdin;
	struct tch_file* 	__stdout;
	struct tch_file*	__stderr;
	void*				__libc_heap;
	size_t				__libc_heapSize;
};

/*!
 * \brief c standard I/O library structure
 *
 *  This Structure contains c standard I/O library functions pointer.
 *  Each pointer is linked to std lib function built statically with platform binary
 *  By default, only minimal set is available.
 *  if more std I/O lib functions are required, platform binary should be rebuilt with
 *  additional macro definition.
 *
 * \see MICRO_LIBSET
 * \see GNUX_LIBSET
 * \see ANSI_LIBSET
 * \see POSIX_LIBSET
 */


typedef struct tch_string_ix_t {
	void* (*memchr)(const void* src,int c, size_t length);
	int (*memcmp)(const void* s1,const void* s2,size_t n);
	void* (*memcpy)(void* out,const void* in,size_t n);
	void* (*memset)(void* dst,int c,size_t length);
	char* (*strcat)(char* dst, const char* src);
	char* (*strchr)(const char* str,int c);
	size_t (*strlen)(const char* str);
	size_t (*strspn)(const char* s1,const char* s2);
	char* (*strtok)(char*__restrict src,const char*__restrict del);
} tch_string_ix;

typedef struct tch_stdlib_ix_t{
	int (*abs)(int x);
	double (*atof)(const char* str);
	float (*atoff)(const char* str);
	int (*atoi)(const char* str);
	long int (*atol)(const char* str);
	void (*free)(void* ptr);
	void* (*malloc)(size_t size);
	int (*rand)(void);
	void (*srand)(unsigned seed);
} tch_stdlib_ix;

typedef struct tch_math_ix_t {
	void* dummy;
} tch_math_ix;


const tch_stdlib_ix* stdlib;
const tch_string_ix* string;
const tch_math_ix* math;


#if defined(__cplusplus)
}
#endif

#endif /* TCH_NCLIB_H_ */
