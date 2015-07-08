/*
 * cdsl.h
 *
 *  Created on: 2015. 5. 14.
 *      Author: innocentevil
 */

#ifndef CDSL_H_
#define CDSL_H_

#if defined(__cplusplus)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#ifndef TEST_SIZE
#define TEST_SIZE 5000
#endif

#define VERBOSE_LOG

#ifdef VERBOSE_LOG
#define log(...) 	printf(__VA_ARGS__)
#else
#define log(...)
#endif


#ifndef BOOL
#define BOOL uint8_t
#define FALSE ((uint8_t) 1 < 0)
#define TRUE  ((uint8_t) 1 > 0)
#endif

typedef void (*cdsl_generic_printer_t) (void*);

#ifndef NULL
#define NULL 	((void*) 0)
#endif



#if defined(__cplusplus)
}
#endif



#endif /* CDSL_H_ */
