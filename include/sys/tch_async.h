/*
 * tch_async.h
 *
 *  Created on: 2014. 8. 8.
 *      Author: innocentevil
 */
/*!
 * \brief Add Asynchronous Task support
 *  operation whose speed is relatively slow than that of CPU, such as I/O operation, could be consuming cpu time enormously,
 *  in this case, asynchronous separation of cpu execution and I/O operation can be possible backed by H/W Feature called DMA(Direct Memory Access)
 *  because Tachyos support Threading and its scheduling as a kernel function, execution of program can yield cpu waiting for completion I/O operation.
 *  if it's guaranteed that current thread is blocked normally and new thread comes to execution stage, current thread can be waken up normally after I/O Complete.
 *  but this is usually not the case, especially I/O speed is not too slow.
 *  Sometimes I/O Operations are completed before current thread is blocked.
 *
 */


#ifndef TCH_ASYNC_H_
#define TCH_ASYNC_H_

#include "tch_Typedef.h"

#if defined(__cplusplus)
extern "C"{
#endif

#define DECL_ASYNC_TASK(fn)      int fn(tch_asyncId id, void* arg)

/**
 */

typedef uaddr_t tch_asyncId;
typedef int (*tch_async_routine)(tch_asyncId id,void* arg);

typedef struct _tch_async_ix_t {
	/*!
	 *
	 */
	tch_asyncId (*create)(size_t q_sz);
	tchStatus (*wait)(tch_asyncId async,int id,tch_async_routine fn,uint32_t timeout,void* arg);
	void (*notify)(tch_asyncId async,int id,tchStatus res);
	void (*destroy)(tch_asyncId async);
};




extern const tch_async_ix* Async;
extern LIST_CMP_FN(tch_async_comp);

#if defined(__cplusplus)
}
#endif

#endif /* TCH_ASYNC_H_ */
