/*
 * tch_ptypes.h
 *
 *  Created on: 2014. 8. 30.
 *      Author: innocentevil
 */

#ifndef TCH_PTYPES_H_
#define TCH_PTYPES_H_

#if defined(__cplusplus)
extern "C"{
#endif


#include <stdint.h>

/*
 *
 *  must be defined properly your target platform
 *
 */
typedef uint32_t uword_t;
typedef uint16_t uhword_t;
typedef int32_t  word_t;
typedef int16_t  hword_t;
typedef void* uaddr_t;

#define WORD_MAX ((uint32_t) -1)


#if defined(__cplusplus)
}
#endif

#endif /* TCH_PTYPES_H_ */
