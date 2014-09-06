/*
 * tch_msgq.h
 *
 *  Created on: 2014. 9. 5.
 *      Author: innocentevil
 */

#ifndef TCH_MSGQ_H_
#define TCH_MSGQ_H_

#if defined(__cplusplus)
extern "C"{
#endif

typedef void* tch_msgQue_id;

/**
 *    CMSIS RTOS Compatible message queue
 */

struct _tch_msgque_ix_t {
	/**
	 *  Create and initiate Message Queue instance
	 *  ##Note : In CMSIS, this function actually has one more arguement, which is thread. however it is pointless
	 *           because there isn't any operation which is dedicated to a given single thread
	 *
	 *  @arg1 message queue definition
	 *  @return queue ID
	 *
	 */
	tch_msgQue_id (*create)(size_t len);

	/**
	 * put message in the message queue (for producer side)
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *  @arg1 message queue id
	 *  @arg2 msg to send
	 *  @arg3 timeout in millisecond
	 *  @return osOK  : the message is put into the queue.
	 *          osErrorResource  : no memory in the queue was available.
	 *          osErrorTimeoutResource  : no memory in the queue was available during the given time limit.
	 *          osErrorParameter  : a parameter is invalid or outside of a permitted
	 */
	tchStatus (*put)(tch_msgQue_id,uint32_t msg,uint32_t millisec);

	/**
	 * Get a Message or Wait for a Message from a Queue
	 * ##Note : In ISR Mode, timeout is forced to 0 and execution is not blocked.
	 *
	 *   @arg1 message queue id
	 *   @arg2 timeout in millisecond
	 *   @return osOK: no message is available in the queue and no timeout was specified.
	 *           osEventTimeout: no message has arrived during the given timeout period.
	 *           osEventMessage: message received, value.p contains the pointer to message.
	 *           osErrorParameter: a parameter is invalid or outside of a permitted range.
	 */
	osEvent (*get)(tch_msgQue_id,uint32_t millisec);

	/*!
	 * \brief destroy msg queue
	 */
	tchStatus (*destroy)(tch_msgQue_id);
};


#if defined(__cplusplus)
}
#endif

#endif /* TCH_MSGQ_H_ */
