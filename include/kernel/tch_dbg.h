/*
 * tch_dbg.h
 *
 *  Created on: 2016. 1. 25.
 *      Author: innocentevil
 */

#ifndef TCH_DBG_H_
#define TCH_DBG_H_

#include "kernel/tch_fs.h"

#if defined(__cplusplus)
extern "C" {
#endif


extern void tch_dbg_init(file* dbg_out);
extern void tch_dbg_flush(void);
extern tch_dbg_api_t Debug_IX;



#if defined(__cplusplus)
}
#endif

#endif /* TCH_DBG_H_ */
