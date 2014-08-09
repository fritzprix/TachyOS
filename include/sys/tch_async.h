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

#if defined(cplusplus)
extern "C"{
#endif


#define DECL_ASYNC_TASK(fn)      int fn(tch_async_id id, void* arg)

/**
 * tachyos에서 I/O는 asynchronous 방식으로 지원될 수 있다. 이는 I/O 동작의 즉시성을
 */

typedef uint32_t tch_async_id;
typedef int (*tch_async_routine)(tch_async_id id,void* arg);
struct tch_async_prior {
	uint8_t VeryHigh;
	uint8_t High;
	uint8_t Normal;
	uint8_t Low;
	uint8_t VeryLow;
};

typedef struct _tch_async_ix_t {
	struct tch_async_prior Prior;
	tch_async_id (*create)(tch_async_routine fn,void* arg,uint8_t prior);
	tchStatus (*start)(tch_async_id id);
	tchStatus (*blockedstart)(tch_async_id id,uint32_t timeout);
	void (*notify)(tch_async_id id,tchStatus res);
	void (*destroy)(tch_async_id id);
}tch_async_ix;

extern const tch_async_ix* Async;
extern LIST_CMP_FN(tch_async_comp);

#if defined(cplusplus)
}
#endif

#endif /* TCH_ASYNC_H_ */
