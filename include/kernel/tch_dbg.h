/*
 * tch_dbg.h
 *
 *  Created on: 2016. 1. 25.
 *      Author: innocentevil
 */

#ifndef TCH_DBG_H_
#define TCH_DBG_H_

#include "tch.h"
#include "kernel/tch_fs.h"

#ifdef __DBG
#define DBG_PRINT(...)			Debug_IX.print(Debug_IX.Normal,tchOK, __VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

#define WARN_PRINT(...)			Debug_IX.print(Debug_IX.Warn, tchOK, __VA_ARGS__)
#define ERR_PRINT(condition, err, ...)	{							\
	if(condition) Debug_IX.print(Debug_IX.Error, err, __VA_ARGS__);	\
}

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
