/*
 * tch_ustdlib.h
 *
 *  Created on: 2014. 7. 13.
 *      Author: innocentevil
 */

#ifndef TCH_USTDLIB_H_
#define TCH_USTDLIB_H_

#include <stddef.h>

typedef struct _tch_stdio_ix_t {

}tch_stdio_ix;

typedef struct _tch_string_ix_t {
	double (*acos)(double x);
	double (*asin)(double x);
}tch_string_ix;

typedef struct _tch_stdlib_ix_t{
	double (*atof)(const char* str);
	int (*atoi)(const char* str);
	long int (*atol)(const char* str);
	void* (*calloc)(size_t nitems,size_t size);
	void (*free)(void* ptr);
	void* (*malloc)(size_t size);
	void* (*realloc)(void* ptr,size_t size);
	char* (*getenv)(const char* name);
	void* (*bsearch)(const void* key,const void* base,size_t nitems,size_t size,int (*compare)(const void*, const void*));
	void (*qsort)(void* base,size_t nitems,size_t size,int (*compare)(const void*,const void*));
	int (*abs)(int x);
	long int (*labs)(long int x);
	int (*rand)(void);
	void (*srand)(uint32_t seed);
}tch_stdlib_ix;

typedef struct _tch_math_ix_t {

}tch_math_ix;

typedef struct _tch_ctype_ix_t {

}tch_ctype_ix;

struct _tch_ustdl_ix_t {
	const tch_stdio_ix* stdio;
	const tch_stdlib_ix* stdlib;
	const tch_string_ix* string;
	const tch_math_ix* math;
	const tch_ctype_ix* ctype;
};

#endif /* TCH_USTDLIB_H_ */
